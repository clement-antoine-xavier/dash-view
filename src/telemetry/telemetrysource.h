#pragma once

#include <QObject>
#include <QString>

#include "telemetry/telemetryvalue.h"

// Abstract stream of tagged vehicle values. Implementations: mock, replay and
// BLE. The source is vehicle-agnostic; the vehicle-specific decoding lives in
// dash-connector.
class TelemetrySource : public QObject
{
    Q_OBJECT

public:
    enum class State {
        Stopped,
        Starting,
        Running,
        Error
    };
    Q_ENUM(State)

    explicit TelemetrySource(QObject *parent = nullptr);
    ~TelemetrySource() override;

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual QString name() const = 0;         // "mock", "replay", "ble"
    virtual QString description() const = 0;  // human readable
    virtual QString label() const = 0;        // short pill label, e.g. "DEMO"

    State state() const { return m_state; }
    QString statusText() const { return m_statusText; }
    QString vehicleName() const { return m_vehicleName; }

signals:
    void valueReceived(const TelemetryValue &value);
    void stateChanged(TelemetrySource::State state, const QString &detail);
    void vehicleNameChanged(const QString &name);

protected:
    void setState(State state, const QString &detail = {});
    void setVehicleName(const QString &name);

private:
    State m_state = State::Stopped;
    QString m_statusText;
    QString m_vehicleName{QStringLiteral("Vehicle")};
};
