#include <QtTest>

#include <cmath>

#include "can/canframe.h"
#include "decode/decoder.h"

namespace {

CanFrame makeFrame(quint32 id, std::initializer_list<quint8> bytes)
{
    CanFrame frame;
    frame.id = id;
    frame.dlc = static_cast<quint8>(bytes.size());
    int i = 0;
    for (quint8 byte : bytes) {
        if (i < 8)
            frame.data[i++] = byte;
    }
    return frame;
}

void writeRaw(const SignalDef &def, quint64 raw, CanFrame &frame)
{
    if (def.bitMode) {
        for (int i = 0; i < def.length; ++i) {
            const int bit = def.offset + i;
            const quint8 mask = static_cast<quint8>(1u << (7 - (bit % 8)));
            if ((raw >> (def.length - 1 - i)) & 1u)
                frame.data[bit / 8] |= mask;
            else
                frame.data[bit / 8] &= static_cast<quint8>(~mask);
        }
    } else {
        for (int i = 0; i < def.length; ++i)
            frame.data[def.offset + i] = static_cast<quint8>((raw >> (8 * (def.length - 1 - i))) & 0xFFu);
    }
}

double decodeValue(SignalId id, const CanFrame &frame, bool *ok = nullptr)
{
    const SignalDef *def = canmap::definition(id);
    double value = qQNaN();
    const bool good = def && decoder::decode(*def, frame, &value);
    if (ok)
        *ok = good;
    return value;
}

} // namespace

class DecoderTest : public QObject
{
    Q_OBJECT

private slots:
    void tableShape();
    void publishedSampleAndQuirks();
    void torqueBitFields();
    void rpmByteOrder();
    void steeringAndWheels();
    void batteryAndFlags();
    void signedBitField();
    void roundTrip();
};

void DecoderTest::tableShape()
{
    const auto &defs = canmap::allSignals();
    QCOMPARE(defs.size(), kSignalCount);
    for (int i = 0; i < defs.size(); ++i)
        QCOMPARE(static_cast<int>(defs.at(i).id), i);

    QCOMPARE(canmap::signalsForId(0x0A5).size(), 3);
    QCOMPARE(canmap::signalsForId(0x254).size(), 4);
    QVERIFY(canmap::signalsForId(0x999).isEmpty());
}

void DecoderTest::publishedSampleAndQuirks()
{
    // Frame from https://thesecretingredient.neocities.org/bmw/can/g29/
    const CanFrame frame = makeFrame(0x3F9, {0x4E, 0xFE, 0x82, 0x3E, 0x81, 0x88, 0x33, 0xA3});

    QCOMPARE(decodeValue(SignalId::CoolantTemp, frame), 81.0);
    QCOMPARE(decodeValue(SignalId::OilTemp, frame), 88.0);

    // The page's "+50" would give 213 rpm; raw * 50 gives 8150. Documented
    // deviation, pending a real log (docs/signals.md).
    QCOMPARE(decodeValue(SignalId::MaxRpm, frame), 8150.0);

    // Gear nibble = 3, published formula raw - 4. Confirms the unsigned
    // interpretation of "Bit Mode" fields.
    QCOMPARE(decodeValue(SignalId::Gear, frame), -1.0);
}

void DecoderTest::torqueBitFields()
{
    // 12-bit field at bits 16..27 (byte 2 + high nibble of byte 3).
    CanFrame frame = makeFrame(0x0A5, {0x00, 0x00, 0x7F, 0xF0, 0x00, 0x00, 0x00, 0x00});
    QCOMPARE(decodeValue(SignalId::EngineTorque, frame), 0.0);

    frame = makeFrame(0x0A5, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
    QCOMPARE(decodeValue(SignalId::EngineTorque, frame), -1023.5);

    frame = makeFrame(0x0A5, {0x00, 0x00, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00});
    QCOMPARE(decodeValue(SignalId::EngineTorque, frame), 1024.0);

    // Gearbox torque occupies bits 28..39: low nibble of byte 3 + byte 4.
    CanFrame gearbox;
    gearbox.id = 0x0A5;
    gearbox.dlc = 8;
    writeRaw(*canmap::definition(SignalId::GearboxTorque), 2047, gearbox); // 2047 / 2 - 1023.5
    QCOMPARE(decodeValue(SignalId::GearboxTorque, gearbox), 0.0);
}

void DecoderTest::rpmByteOrder()
{
    // Bytes 5..6 = 0x3E80 = 16000; /4 = 4000 rpm.
    const CanFrame frame = makeFrame(0x0A5, {0x00, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x80, 0x00});
    QCOMPARE(decodeValue(SignalId::Rpm, frame), 4000.0);
}

void DecoderTest::steeringAndWheels()
{
    // 22.75 * 1440 = 32760 -> 0 degrees.
    const CanFrame steering = makeFrame(0x301, {0x00, 0x00, 0x7F, 0xF8, 0x00, 0x00, 0x00, 0x00});
    QVERIFY(std::abs(decodeValue(SignalId::Steering, steering)) < 1e-9);

    // 0x8000 / 64 - 511.98001 = 0.01999 km/h.
    const CanFrame wheels = makeFrame(0x254, {0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00});
    const double speed = decodeValue(SignalId::WheelFrontLeft, wheels);
    QVERIFY2(std::abs(speed - 0.01999) < 0.01, qPrintable(QString::number(speed)));
}

void DecoderTest::batteryAndFlags()
{
    // 12-bit field at bit 1: raw 943 -> 14.145 V with *15/1000.
    CanFrame battery;
    battery.id = 0x281;
    battery.dlc = 8;
    writeRaw(*canmap::definition(SignalId::BatteryVoltage), 943, battery);
    QVERIFY(std::abs(decodeValue(SignalId::BatteryVoltage, battery) - 14.145) < 1e-9);

    // High beam: bit 5 -> byte 0 bit 2.
    const CanFrame beam = makeFrame(0x1EE, {0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
    QCOMPARE(decodeValue(SignalId::HighBeam, beam), 1.0);

    // Turn left bit 4, turn right bit 5 (byte 0 bits 3 and 2).
    const CanFrame turns = makeFrame(0x1F6, {0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
    QCOMPARE(decodeValue(SignalId::TurnLeft, turns), 1.0);
    QCOMPARE(decodeValue(SignalId::TurnRight, turns), 1.0);

    // Gear 6 is raw 10 (0xA) in the high nibble of byte 6.
    const CanFrame gear = makeFrame(0x3F9, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x00});
    QCOMPARE(decodeValue(SignalId::Gear, gear), 6.0);
}

void DecoderTest::signedBitField()
{
    SignalDef def{};
    def.bitMode = true;
    def.offset = 0;
    def.length = 4;
    def.signedRaw = true;
    def.mul = 1.0;
    def.div = 1.0;

    const CanFrame minusOne = makeFrame(0x123, {0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
    double value = 0.0;
    QVERIFY(decoder::decode(def, minusOne, &value));
    QCOMPARE(value, -1.0);

    const CanFrame seven = makeFrame(0x123, {0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
    QVERIFY(decoder::decode(def, seven, &value));
    QCOMPARE(value, 7.0);
}

void DecoderTest::roundTrip()
{
    struct Case {
        SignalId id;
        double value;
        double tolerance;
    };

    const Case cases[] = {
        {SignalId::Rpm, 4000.0, 2.0},
        {SignalId::Throttle, 55.5, 0.05},
        {SignalId::Accelerator, 88.0, 0.05},
        {SignalId::CoolantTemp, 90.0, 0.5},
        {SignalId::Steering, -45.5, 0.05},
        {SignalId::BatteryVoltage, 14.2, 0.01},
        {SignalId::Gear, 6.0, 1e-9},
        {SignalId::WheelFrontLeft, 120.25, 0.02},
        {SignalId::FuelRange, 230.0, 0.1},
        {SignalId::AccelX, -0.4, 0.001},
        {SignalId::YawRate, 12.5, 0.01},
        {SignalId::BrakePercent, 42.0, 0.5},
        {SignalId::MaxRpm, 7000.0, 1e-9},
    };

    for (const Case &item : cases) {
        const SignalDef *def = canmap::definition(item.id);
        QVERIFY(def);

        CanFrame frame;
        frame.id = def->canId;
        frame.dlc = 8;
        writeRaw(*def, decoder::encodeRaw(*def, item.value), frame);

        double decoded = 0.0;
        QVERIFY2(decoder::decode(*def, frame, &decoded), qPrintable(QString::fromLatin1(def->key)));
        QVERIFY2(std::abs(decoded - item.value) <= item.tolerance,
                 qPrintable(QStringLiteral("%1: expected %2, got %3")
                                .arg(QString::fromLatin1(def->key))
                                .arg(item.value)
                                .arg(decoded)));
    }
}

QTEST_GUILESS_MAIN(DecoderTest)

#include "tst_decoder.moc"
