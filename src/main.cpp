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
#include <QTimer>
#include <QWindow>

#include "can/democansource.h"
#include "vehicle/vehiclestate.h"

namespace {

QString stateName(CanSource::State state)
{
    switch (state) {
    case CanSource::State::Stopped:
        return QStringLiteral("STOPPED");
    case CanSource::State::Starting:
        return QStringLiteral("STARTING");
    case CanSource::State::Running:
        return QStringLiteral("RUNNING");
    case CanSource::State::Error:
        return QStringLiteral("ERROR");
    }
    return QStringLiteral("UNKNOWN");
}

} // namespace

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("DashView"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("dashview.dev"));
    QCoreApplication::setApplicationName(QStringLiteral("Dash View"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("Dash View"));
    QGuiApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("BMW G29 PT-CAN dashboard (mock data build)"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption sourceOption(
        QStringLiteral("source"),
        QStringLiteral("Frame source. Currently: demo."),
        QStringLiteral("name"),
        QStringLiteral("demo"));
    QCommandLineOption screenshotOption(
        QStringLiteral("screenshot"),
        QStringLiteral("Render the dashboard to a PNG file and exit."),
        QStringLiteral("file"));
    QCommandLineOption screenshotDelayOption(
        QStringLiteral("screenshot-delay"),
        QStringLiteral("Delay before taking the screenshot, in milliseconds."),
        QStringLiteral("ms"),
        QStringLiteral("2500"));
    QCommandLineOption fullscreenOption(
        QStringLiteral("fullscreen"),
        QStringLiteral("Start in full screen."));
    QCommandLineOption debugOption(
        QStringLiteral("debug"),
        QStringLiteral("Open the decoder debug overlay on start."));

    parser.addOptions({sourceOption, screenshotOption, screenshotDelayOption, fullscreenOption, debugOption});
    parser.process(app);

    if (parser.value(sourceOption) != QStringLiteral("demo")) {
        qCritical().noquote() << "Unsupported --source" << parser.value(sourceOption)
                              << "(this build only ships the 'demo' source)";
        return 2;
    }

    VehicleState vehicle;
    DemoCanSource demo;
    vehicle.setSourceName(demo.description());

    QObject::connect(&demo, &CanSource::frameReceived, &vehicle, &VehicleState::ingestFrame);
    QObject::connect(&demo, &CanSource::stateChanged, &vehicle,
                     [&vehicle](CanSource::State state, const QString &) {
                         vehicle.setSourceState(stateName(state));
                     });
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &demo, &DemoCanSource::stop);

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

    demo.start();

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
