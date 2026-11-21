#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTextStream>
#include <QTimer>
#include <QWindow>

#include <memory>

#include "telemetry/bletelemetrysource.h"
#include "telemetry/mocktelemetrysource.h"
#include "telemetry/replaytelemetrysource.h"
#include "telemetry/signaltag.h"
#include "telemetry/telemetryrecorder.h"
#include "telemetry/telemetrysource.h"
#include "vehicle/vehiclestate.h"

namespace {

QString stateName(TelemetrySource::State state)
{
    switch (state) {
    case TelemetrySource::State::Stopped:
        return QStringLiteral("STOPPED");
    case TelemetrySource::State::Starting:
        return QStringLiteral("STARTING");
    case TelemetrySource::State::Running:
        return QStringLiteral("RUNNING");
    case TelemetrySource::State::Error:
        return QStringLiteral("ERROR");
    }
    return QStringLiteral("UNKNOWN");
}

void printTagCatalog()
{
    QTextStream out(stdout);
    out << "TAG   KEY                  LABEL                UNIT\n";
    for (const sig::SignalInfo &info : sig::catalog()) {
        out << sig::hex(info.tag).leftJustified(6)
            << QString::fromLatin1(info.key).leftJustified(21)
            << QString::fromUtf8(info.label).leftJustified(21)
            << QString::fromUtf8(info.unit) << '\n';
    }
    out << "\nCustom tags 0x80..0xEF are recorded and shown in the debug overlay.\n";
    out.flush();
}

} // namespace

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("DashView"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("dashview.dev"));
    QCoreApplication::setApplicationName(QStringLiteral("Dash View"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("Dash View"));
    QGuiApplication::setApplicationVersion(QStringLiteral("0.2.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Vehicle-agnostic tagged telemetry dashboard"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption sourceOption(
        QStringLiteral("source"),
        QStringLiteral("Telemetry source: mock, replay or ble."),
        QStringLiteral("kind"),
        QStringLiteral("mock"));
    QCommandLineOption replayOption(
        QStringLiteral("replay"),
        QStringLiteral("JSON Lines recording to replay."),
        QStringLiteral("file"));
    QCommandLineOption loopOption(
        QStringLiteral("loop"),
        QStringLiteral("Loop the replay when it reaches the end."));
    QCommandLineOption speedOption(
        QStringLiteral("speed"),
        QStringLiteral("Replay speed factor (default 1.0)."),
        QStringLiteral("factor"),
        QStringLiteral("1.0"));
    QCommandLineOption recordOption(
        QStringLiteral("record"),
        QStringLiteral("Record the telemetry stream to a JSON Lines file."),
        QStringLiteral("file"));
    QCommandLineOption bleNameOption(
        QStringLiteral("ble-name"),
        QStringLiteral("BLE device name filter (default: DashView / service UUID)."),
        QStringLiteral("substring"));
    QCommandLineOption vehicleNameOption(
        QStringLiteral("vehicle-name"),
        QStringLiteral("Override the vehicle name shown in the UI."),
        QStringLiteral("name"));
    QCommandLineOption listTagsOption(
        QStringLiteral("list-tags"),
        QStringLiteral("Print the standard telemetry tag catalog and exit."));
    QCommandLineOption fullscreenOption(
        QStringLiteral("fullscreen"),
        QStringLiteral("Start in full screen."));
    QCommandLineOption debugOption(
        QStringLiteral("debug"),
        QStringLiteral("Open the telemetry debug overlay on start."));
    QCommandLineOption screenshotOption(
        QStringLiteral("screenshot"),
        QStringLiteral("Render the dashboard to a PNG file and exit."),
        QStringLiteral("file"));
    QCommandLineOption screenshotDelayOption(
        QStringLiteral("screenshot-delay"),
        QStringLiteral("Delay before taking the screenshot, in milliseconds."),
        QStringLiteral("ms"),
        QStringLiteral("2500"));

    parser.addOptions({sourceOption, replayOption, loopOption, speedOption, recordOption,
                       bleNameOption, vehicleNameOption, listTagsOption, fullscreenOption,
                       debugOption, screenshotOption, screenshotDelayOption});
    parser.process(app);

    if (parser.isSet(listTagsOption)) {
        printTagCatalog();
        return 0;
    }

    QString sourceKind = parser.value(sourceOption).toLower();
    if (!parser.isSet(sourceOption)) {
        if (parser.isSet(replayOption))
            sourceKind = QStringLiteral("replay");
        else if (parser.isSet(bleNameOption))
            sourceKind = QStringLiteral("ble");
    }

    if (sourceKind != QStringLiteral("mock")
        && sourceKind != QStringLiteral("replay")
        && sourceKind != QStringLiteral("ble")) {
        qCritical().noquote() << "Unknown --source" << sourceKind << "(use mock, replay or ble)";
        return 2;
    }

    const double speed = parser.value(speedOption).toDouble();
    if (!(speed > 0.0)) {
        qCritical() << "Invalid --speed" << parser.value(speedOption);
        return 2;
    }

    const bool loop = parser.isSet(loopOption);
    const QString replayPath = parser.value(replayOption);
    const QString recordPath = parser.value(recordOption);
    const QString nameOverride = parser.value(vehicleNameOption).trimmed();

    std::unique_ptr<TelemetrySource> source;
    if (sourceKind == QStringLiteral("mock")) {
        source = std::make_unique<MockTelemetrySource>();
    } else if (sourceKind == QStringLiteral("replay")) {
        if (replayPath.isEmpty()) {
            qCritical() << "--source replay requires --replay <file>";
            return 2;
        }
        source = std::make_unique<ReplayTelemetrySource>(replayPath, loop, speed);
    } else {
        source = std::make_unique<BleTelemetrySource>(parser.value(bleNameOption));
    }

    VehicleState vehicle;
    TelemetryRecorder recorder;

    vehicle.setSourceName(source->description());
    vehicle.setSourceLabel(source->label());
    vehicle.setVehicleName(nameOverride.isEmpty() ? source->vehicleName() : nameOverride);

    QObject::connect(source.get(), &TelemetrySource::valueReceived,
                     &vehicle, &VehicleState::ingest);
    QObject::connect(source.get(), &TelemetrySource::stateChanged,
                     &vehicle, [&vehicle](TelemetrySource::State state, const QString &) {
                         vehicle.setSourceState(stateName(state));
                     });
    QObject::connect(source.get(), &TelemetrySource::vehicleNameChanged,
                     &vehicle, [&vehicle, nameOverride](const QString &name) {
                         if (nameOverride.isEmpty())
                             vehicle.setVehicleName(name);
                     });

    const bool recordRequested = !recordPath.isEmpty();
    if (recordRequested) {
        QObject::connect(source.get(), &TelemetrySource::valueReceived,
                         &recorder, &TelemetryRecorder::record);
        QObject::connect(&vehicle, &VehicleState::vehicleNameChanged,
                         &recorder, [&vehicle, &recorder]() {
                             recorder.writeMeta(vehicle.vehicleName());
                         });
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("Vehicle"), &vehicle);
    engine.rootContext()->setContextProperty(QStringLiteral("StartDebug"), parser.isSet(debugOption));
    engine.loadFromModule(QStringLiteral("DashView"), QStringLiteral("Main"));

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load the QML interface";
        return 1;
    }

    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
    if (!window) {
        qCritical() << "Root QML object is not a Window";
        return 1;
    }

    if (parser.isSet(fullscreenOption))
        window->setVisibility(QWindow::FullScreen);

    QObject::connect(&app, &QCoreApplication::aboutToQuit, source.get(), [&source]() {
        if (source)
            source->stop();
    });
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &recorder, &TelemetryRecorder::stop);

    if (recordRequested) {
        if (recorder.start(recordPath, vehicle.vehicleName(), source->label()))
            qInfo().noquote() << "Recording to" << QFileInfo(recordPath).absoluteFilePath();
        else
            qWarning().noquote() << "Could not open recording file" << recordPath;
    }

    source->start();

    if (parser.isSet(screenshotOption)) {
        const QString path = QFileInfo(parser.value(screenshotOption)).absoluteFilePath();
        const int delay = qBound(100, parser.value(screenshotDelayOption).toInt(), 60000);

        QDir().mkpath(QFileInfo(path).absolutePath());

        QTimer::singleShot(delay, &app, [window, path]() {
            const QImage image = window->grabWindow();
            if (image.isNull()) {
                qCritical() << "Screenshot failed: grabWindow() returned a null image";
                QCoreApplication::exit(3);
                return;
            }
            if (!image.save(path)) {
                qCritical() << "Screenshot failed: could not write" << path;
                QCoreApplication::exit(3);
                return;
            }
            qInfo().noquote() << "Screenshot written to" << path
                              << QStringLiteral("(%1x%2)").arg(image.width()).arg(image.height());
            QCoreApplication::quit();
        });
    }

    return app.exec();
}
