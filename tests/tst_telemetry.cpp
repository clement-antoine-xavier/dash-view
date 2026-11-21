#include <QtTest>

#include <QFile>
#include <QSet>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTextStream>

#include "telemetry/mocktelemetrysource.h"
#include "telemetry/replaytelemetrysource.h"
#include "telemetry/signaltag.h"
#include "telemetry/telemetrycodec.h"
#include "telemetry/telemetryrecorder.h"
#include "telemetry/telemetryvalue.h"
#include "vehicle/vehiclestate.h"

namespace {

TelemetryValue spyValue(const QSignalSpy &spy, int index)
{
    return qvariant_cast<TelemetryValue>(spy.at(index).at(0));
}

} // namespace

class TelemetryTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void tagCatalog();
    void codecFloat();
    void codecIntAndBool();
    void codecNotification();
    void recorderAndReplay();
    void vehicleState();
    void mockSource();
};

void TelemetryTest::initTestCase()
{
    qRegisterMetaType<TelemetryValue>("TelemetryValue");
}

void TelemetryTest::tagCatalog()
{
    const auto &catalog = sig::catalog();
    QCOMPARE(catalog.size(), 44);

    QSet<quint8> tags;
    for (const sig::SignalInfo &info : catalog) {
        QVERIFY2(!tags.contains(info.tag), qPrintable(sig::hex(info.tag)));
        tags.insert(info.tag);
        QVERIFY(info.known);
        QVERIFY(info.key && *info.key);
        QVERIFY(info.timeoutMs > 0);
    }

    QVERIFY(sig::isKnown(sig::Rpm));
    QCOMPARE(sig::key(sig::Rpm), QStringLiteral("rpm"));
    QCOMPARE(sig::label(sig::Rpm), QStringLiteral("RPM"));
    QCOMPARE(sig::unit(sig::Speed), QStringLiteral("km/h"));
    QCOMPARE(sig::hex(0x0A), QStringLiteral("0x0A"));
    QVERIFY(sig::info(sig::HighBeam).boolean);

    QVERIFY(!sig::isKnown(0x80));
    QCOMPARE(sig::label(0x80), QStringLiteral("Custom 0x80"));
    QCOMPARE(sig::logName(0x80), QStringLiteral("custom_80"));
    QCOMPARE(sig::timeoutMs(0x80), quint16(5000));
}

void TelemetryTest::codecFloat()
{
    const TelemetryValue input{sig::Speed, 123.5};
    const QByteArray record = telemetry::encode(input);

    QCOMPARE(record.size(), telemetry::RecordSize);
    QCOMPARE(quint8(record.at(0)), sig::Speed);
    QCOMPARE(quint8(record.at(1)), quint8(telemetry::WireType::Float32));

    TelemetryValue output;
    QVERIFY(telemetry::decodeRecord(record, &output));
    QCOMPARE(output.tag, sig::Speed);
    QVERIFY(qAbs(output.value - 123.5) < 1e-6);

    QVERIFY(!telemetry::decodeRecord(record.left(5), &output));

    TelemetryValue negative{sig::YawRate, -42.25};
    QVERIFY(telemetry::decodeRecord(telemetry::encode(negative), &output));
    QVERIFY(qAbs(output.value + 42.25) < 1e-6);
}

void TelemetryTest::codecIntAndBool()
{
    TelemetryValue output;

    const TelemetryValue gear{sig::Gear, 6};
    QVERIFY(telemetry::decodeRecord(telemetry::encode(gear, telemetry::WireType::Int32), &output));
    QCOMPARE(output.tag, sig::Gear);
    QCOMPARE(output.value, 6.0);

    const TelemetryValue reverse{sig::Gear, -1};
    QVERIFY(telemetry::decodeRecord(telemetry::encode(reverse, telemetry::WireType::Int32), &output));
    QCOMPARE(output.value, -1.0);

    const TelemetryValue odo{sig::Odometer, 123456};
    QVERIFY(telemetry::decodeRecord(telemetry::encode(odo, telemetry::WireType::UInt32), &output));
    QCOMPARE(output.value, 123456.0);

    const TelemetryValue beam{sig::HighBeam, 1};
    QVERIFY(telemetry::decodeRecord(telemetry::encode(beam, telemetry::WireType::Bool), &output));
    QCOMPARE(output.value, 1.0);

    const TelemetryValue beamOff{sig::HighBeam, 0};
    QVERIFY(telemetry::decodeRecord(telemetry::encode(beamOff, telemetry::WireType::Bool), &output));
    QCOMPARE(output.value, 0.0);

    QByteArray invalidTag = telemetry::encode(beam);
    invalidTag[0] = char(0);
    QVERIFY(!telemetry::decodeRecord(invalidTag, &output));

    QByteArray invalidType = telemetry::encode(beam);
    invalidType[1] = char(9);
    QVERIFY(!telemetry::decodeRecord(invalidType, &output));
}

void TelemetryTest::codecNotification()
{
    QByteArray payload;
    payload += telemetry::encode({sig::Rpm, 858.0});
    payload += telemetry::encode({sig::Speed, 88.5});
    payload += telemetry::encode({sig::Gear, 4}, telemetry::WireType::Int32);

    QCOMPARE(payload.size(), 18);

    const QVector<TelemetryValue> values = telemetry::decodeNotification(payload);
    QCOMPARE(values.size(), 3);
    QCOMPARE(values.at(0).tag, sig::Rpm);
    QCOMPARE(values.at(1).tag, sig::Speed);
    QCOMPARE(values.at(2).tag, sig::Gear);
    QCOMPARE(values.at(2).value, 4.0);

    // Trailing bytes shorter than one record are ignored.
    const QVector<TelemetryValue> padded = telemetry::decodeNotification(payload + QByteArray(2, '\0'));
    QCOMPARE(padded.size(), 3);
}

void TelemetryTest::recorderAndReplay()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("drive.jsonl"));

    TelemetryRecorder recorder;
    QVERIFY(recorder.start(path, QStringLiteral("Test vehicle"), QStringLiteral("TEST")));
    recorder.record({sig::Rpm, 858.0});
    recorder.record({sig::Speed, 50.0});
    recorder.record({sig::Gear, 3});
    recorder.record({0x80, 7.5});
    recorder.writeMeta(QStringLiteral("Renamed vehicle"));
    recorder.stop();

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray contents = file.readAll();
    QVERIFY(contents.contains("\"type\":\"meta\""));
    QVERIFY(contents.contains("Renamed vehicle"));
    QVERIFY(contents.contains("\"name\":\"rpm\""));

    ReplayTelemetrySource replay(path, false, 2.0);
    QSignalSpy spy(&replay, &TelemetrySource::valueReceived);
    replay.start();

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 4, 5000);
    QCOMPARE(replay.vehicleName(), QStringLiteral("Renamed vehicle"));

    QSet<quint8> tags;
    for (int i = 0; i < spy.count(); ++i)
        tags.insert(spyValue(spy, i).tag);

    QVERIFY(tags.contains(sig::Rpm));
    QVERIFY(tags.contains(sig::Speed));
    QVERIFY(tags.contains(sig::Gear));
    QVERIFY(tags.contains(0x80));

    replay.stop();
}

void TelemetryTest::vehicleState()
{
    VehicleState state;
    state.setVehicleName(QStringLiteral("Test car"));
    state.setSourceName(QStringLiteral("Unit test"));
    state.setSourceLabel(QStringLiteral("TEST"));
    QCOMPARE(state.vehicleName(), QStringLiteral("Test car"));

    state.ingest({sig::Rpm, 1000.0});
    state.ingest({sig::Speed, 50.0});
    state.ingest({sig::Gear, 3});
    state.ingest({0x80, 7.5});

    QTRY_VERIFY_WITH_TIMEOUT(state.values().contains(QStringLiteral("rpm")), 1000);

    const QVariantMap values = state.values();
    QCOMPARE(values.value(QStringLiteral("rpm")).toDouble(), 1000.0);
    QCOMPARE(values.value(QStringLiteral("speed")).toDouble(), 50.0);
    QCOMPARE(values.value(QStringLiteral("gearLabel")).toString(), QStringLiteral("3"));

    QCOMPARE(state.debugRecent().size(), 4);
    QCOMPARE(state.debugRecent().first().toMap().value(QStringLiteral("tag")).toString(),
             QStringLiteral("0x80"));

    bool foundCustom = false;
    const QVariantList rows = state.debugRows();
    for (const QVariant &rowValue : rows) {
        const QVariantMap row = rowValue.toMap();
        if (row.value(QStringLiteral("tag")).toString() == QStringLiteral("0x80")) {
            foundCustom = true;
            QCOMPARE(row.value(QStringLiteral("label")).toString(), QStringLiteral("Custom 0x80"));
            QCOMPARE(row.value(QStringLiteral("value")).toDouble(), 7.5);
        }
    }
    QVERIFY(foundCustom);

    // RPM times out after 1 s and disappears from the QML map.
    QTest::qWait(1200);
    QTRY_VERIFY_WITH_TIMEOUT(!state.values().contains(QStringLiteral("rpm")), 1000);
}

void TelemetryTest::mockSource()
{
    MockTelemetrySource mock;
    QSignalSpy spy(&mock, &TelemetrySource::valueReceived);

    mock.start();
    QTRY_VERIFY_WITH_TIMEOUT(spy.count() > 20, 3000);

    QSet<quint8> tags;
    for (int i = 0; i < spy.count(); ++i)
        tags.insert(spyValue(spy, i).tag);

    QVERIFY(tags.contains(sig::Rpm));
    QVERIFY(tags.contains(sig::Speed));
    QVERIFY(tags.contains(sig::Gear));
    QVERIFY(tags.contains(sig::MaxRpm));

    mock.stop();
    QCOMPARE(mock.vehicleName(), QStringLiteral("Demo vehicle"));
}

QTEST_GUILESS_MAIN(TelemetryTest)

#include "tst_telemetry.moc"
