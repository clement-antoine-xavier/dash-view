#pragma once

#include <QElapsedTimer>
#include <QTimer>

#include "can/cansource.h"
#include "decode/signaldef.h"

// Synthetic G29 drive cycle. Frames use the real CAN identifiers and payload
// layout, so the demo exercises the same decoder and UI path as live data.
class DemoCanSource final : public CanSource
{
    Q_OBJECT

public:
    explicit DemoCanSource(QObject *parent = nullptr);

    void start() override;
    void stop() override;
    QString name() const override;
    QString description() const override;

private slots:
    void tick();

private:
    void emitFrame(quint16 id, const std::array<quint8, 8> &data, qint64 timestampMs);
    void writeSignal(SignalId id, double value, std::array<quint8, 8> &data) const;

    QTimer m_timer;
    QElapsedTimer m_clock;
    quint64 m_tick = 0;
    double m_fuelUsed = 0.0;
};
