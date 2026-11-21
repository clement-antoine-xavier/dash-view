#include "telemetry/telemetrysource.h"

TelemetrySource::TelemetrySource(QObject *parent)
    : QObject(parent)
{
}

TelemetrySource::~TelemetrySource() = default;

void TelemetrySource::setState(State state, const QString &detail)
{
    if (m_state == state && m_statusText == detail)
        return;
    m_state = state;
    m_statusText = detail;
    emit stateChanged(m_state, m_statusText);
}

void TelemetrySource::setVehicleName(const QString &name)
{
    const QString cleaned = name.trimmed();
    if (cleaned.isEmpty() || cleaned == m_vehicleName)
        return;
    m_vehicleName = cleaned;
    emit vehicleNameChanged(m_vehicleName);
}
