#pragma once

#include <QElapsedTimer>
#include <QString>
#include <QTimer>
#include <QVector>

#include "telemetry/signaltag.h"
#include "telemetry/telemetrysource.h"

// Plays back a JSON Lines recording produced by TelemetryRecorder.
class ReplayTelemetrySource final : public TelemetrySource
{
    Q_OBJECT

public:
    ReplayTelemetrySource(QString path, bool loop, double speed, QObject *parent = nullptr);

    void start() override;
    void stop() override;
    QString name() const override;
    QString description() const override;
    QString label() const override;

private slots:
    void tick();

private:
    struct Event
    {
        qint64 t = 0;
        quint8 tag = 0;
        double value = 0.0;
    };

    bool load();
    void stopWithError(const QString &message);

    QVector<Event> m_events;
    QString m_path;
    QString m_loadError;
    QString m_vehicleName;
    bool m_loop = false;
    double m_speed = 1.0;
    bool m_loaded = false;
    int m_index = 0;

    QTimer m_timer;
    QElapsedTimer m_clock;
};
