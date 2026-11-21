#pragma once

#include <QElapsedTimer>
#include <QFile>
#include <QObject>
#include <QString>

#include "telemetry/telemetryvalue.h"

class QJsonObject;

// Writes the app's tagged telemetry stream as JSON Lines:
//
//   {"type":"meta","version":1,"vehicle":"Demo vehicle","source":"DEMO"}
//   {"t":120,"tag":1,"name":"rpm","v":858}
//
// Timestamps are milliseconds since recording start. The same format is read
// by ReplayTelemetrySource.
class TelemetryRecorder : public QObject
{
    Q_OBJECT

public:
    explicit TelemetryRecorder(QObject *parent = nullptr);
    ~TelemetryRecorder() override;

    bool start(const QString &path, const QString &vehicleName, const QString &sourceLabel);
    void stop();
    bool isRecording() const { return m_active; }

public slots:
    void record(const TelemetryValue &value);
    void writeMeta(const QString &vehicleName);

private:
    void writeMetaLine();
    void writeLine(const QJsonObject &object);

    QFile m_file;
    QElapsedTimer m_clock;
    QString m_vehicleName{QStringLiteral("Vehicle")};
    QString m_sourceLabel;
    int m_sinceFlush = 0;
    bool m_active = false;
};
