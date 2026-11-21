#pragma once

#include <QElapsedTimer>
#include <QTimer>

#include "telemetry/signaltag.h"
#include "telemetry/telemetrysource.h"

// Synthetic drive cycle. Emits generic telemetry tags, so it exercises the
// same path as the Arduino's tagged BLE stream.
class MockTelemetrySource final : public TelemetrySource
{
    Q_OBJECT

public:
    explicit MockTelemetrySource(QObject *parent = nullptr);

    void start() override;
    void stop() override;
    QString name() const override;
    QString description() const override;
    QString label() const override;

private slots:
    void tick();

private:
    void emitValue(sig::Tag tag, double value);

    QTimer m_timer;
    QElapsedTimer m_clock;
    quint64 m_tick = 0;
    double m_fuelUsed = 0.0;
};
