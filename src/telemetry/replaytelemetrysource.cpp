#include "telemetry/replaytelemetrysource.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTextStream>

#include <algorithm>
#include <cmath>

ReplayTelemetrySource::ReplayTelemetrySource(QString path, bool loop, double speed, QObject *parent)
    : TelemetrySource(parent)
    , m_path(std::move(path))
    , m_loop(loop)
    , m_speed(speed > 0.0 ? speed : 1.0)
{
    m_timer.setInterval(5);
    connect(&m_timer, &QTimer::timeout, this, &ReplayTelemetrySource::tick);
}

void ReplayTelemetrySource::start()
{
    if (state() == State::Running)
        return;

    if (!m_loaded) {
        if (!load()) {
            stopWithError(m_loadError);
            return;
        }
    }

    if (m_events.isEmpty()) {
        stopWithError(QStringLiteral("Log has no telemetry values"));
        return;
    }

    m_index = 0;
    setVehicleName(m_vehicleName);
    setState(State::Starting, QStringLiteral("Starting replay"));
    m_clock.start();
    m_timer.start();
    setState(State::Running, QStringLiteral("Replaying %1 values").arg(m_events.size()));
}

void ReplayTelemetrySource::stop()
{
    m_timer.stop();
    if (state() != State::Error)
        setState(State::Stopped, QStringLiteral("Stopped"));
}

QString ReplayTelemetrySource::name() const
{
    return QStringLiteral("replay");
}

QString ReplayTelemetrySource::description() const
{
    return QStringLiteral("Replay · ") + QFileInfo(m_path).fileName();
}

QString ReplayTelemetrySource::label() const
{
    return QStringLiteral("REPLAY");
}

void ReplayTelemetrySource::tick()
{
    if (m_events.isEmpty())
        return;

    const qint64 limit = static_cast<qint64>(m_clock.elapsed() * m_speed);

    while (m_index < m_events.size() && m_events.at(m_index).t <= limit) {
        TelemetryValue value;
        value.tag = m_events.at(m_index).tag;
        value.value = m_events.at(m_index).value;
        emit valueReceived(value);
        ++m_index;
    }

    if (m_index < m_events.size())
        return;

    if (m_loop) {
        m_clock.restart();
        m_index = 0;
    } else {
        m_timer.stop();
        setState(State::Stopped, QStringLiteral("End of replay"));
    }
}

bool ReplayTelemetrySource::load()
{
    m_events.clear();
    m_loadError.clear();

    QFile file(m_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_loadError = QStringLiteral("Cannot open %1").arg(m_path);
        return false;
    }

    QTextStream stream(&file);
    int skipped = 0;

    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty())
            continue;

        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(line.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError || !document.isObject()) {
            ++skipped;
            continue;
        }

        const QJsonObject object = document.object();
        if (object.value(QStringLiteral("type")).toString() == QStringLiteral("meta")) {
            const QString vehicle = object.value(QStringLiteral("vehicle")).toString().trimmed();
            if (!vehicle.isEmpty())
                m_vehicleName = vehicle;
            continue;
        }

        const double t = object.value(QStringLiteral("t")).toDouble(-1.0);
        const int tag = object.value(QStringLiteral("tag")).toInt(-1);
        const double value = object.value(QStringLiteral("v")).toDouble(qQNaN());

        if (t < 0.0 || tag <= 0 || tag > 255 || !std::isfinite(value)) {
            ++skipped;
            continue;
        }

        Event event;
        event.t = static_cast<qint64>(t);
        event.tag = static_cast<quint8>(tag);
        event.value = value;
        m_events.append(event);
    }

    std::sort(m_events.begin(), m_events.end(), [](const Event &a, const Event &b) {
        return a.t < b.t;
    });

    if (m_vehicleName.trimmed().isEmpty())
        m_vehicleName = QStringLiteral("Replay ") + QFileInfo(m_path).completeBaseName();

    m_loaded = true;

    if (m_events.isEmpty()) {
        m_loadError = skipped > 0
            ? QStringLiteral("No valid values in %1 (%2 lines skipped)").arg(m_path).arg(skipped)
            : QStringLiteral("No values in %1").arg(m_path);
        return false;
    }

    return true;
}

void ReplayTelemetrySource::stopWithError(const QString &message)
{
    m_timer.stop();
    setState(State::Error, message);
}
