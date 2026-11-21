#include "telemetry/telemetryrecorder.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

#include "telemetry/signaltag.h"

TelemetryRecorder::TelemetryRecorder(QObject *parent)
    : QObject(parent)
{
}

TelemetryRecorder::~TelemetryRecorder()
{
    stop();
}

bool TelemetryRecorder::start(const QString &path, const QString &vehicleName, const QString &sourceLabel)
{
    stop();

    if (path.isEmpty())
        return false;

    const QFileInfo info(path);
    if (!info.absolutePath().isEmpty())
        QDir().mkpath(info.absolutePath());

    m_file.setFileName(path);
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        m_file.setFileName(QString());
        return false;
    }

    m_vehicleName = vehicleName.trimmed().isEmpty() ? QStringLiteral("Vehicle") : vehicleName.trimmed();
    m_sourceLabel = sourceLabel;
    m_sinceFlush = 0;
    m_active = true;
    m_clock.start();
    writeMetaLine();
    return true;
}

void TelemetryRecorder::stop()
{
    if (!m_active)
        return;

    m_file.flush();
    m_file.close();
    m_active = false;
}

void TelemetryRecorder::record(const TelemetryValue &value)
{
    if (!m_active || value.tag == 0)
        return;

    QJsonObject object;
    object.insert(QStringLiteral("t"), static_cast<double>(m_clock.elapsed()));
    object.insert(QStringLiteral("tag"), static_cast<int>(value.tag));
    object.insert(QStringLiteral("v"), value.value);

    const QString key = sig::key(value.tag);
    if (!key.isEmpty())
        object.insert(QStringLiteral("name"), key);

    writeLine(object);
}

void TelemetryRecorder::writeMeta(const QString &vehicleName)
{
    if (!m_active)
        return;

    const QString cleaned = vehicleName.trimmed();
    if (cleaned.isEmpty() || cleaned == m_vehicleName)
        return;

    m_vehicleName = cleaned;
    writeMetaLine();
}

void TelemetryRecorder::writeMetaLine()
{
    QJsonObject object;
    object.insert(QStringLiteral("type"), QStringLiteral("meta"));
    object.insert(QStringLiteral("version"), 1);
    object.insert(QStringLiteral("vehicle"), m_vehicleName);
    object.insert(QStringLiteral("source"), m_sourceLabel);
    writeLine(object);
}

void TelemetryRecorder::writeLine(const QJsonObject &object)
{
    if (!m_active)
        return;

    m_file.write(QJsonDocument(object).toJson(QJsonDocument::Compact));
    m_file.write("\n");

    if (++m_sinceFlush >= 64) {
        m_file.flush();
        m_sinceFlush = 0;
    }
}
