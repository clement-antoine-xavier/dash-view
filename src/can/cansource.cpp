#include "can/cansource.h"

CanSource::CanSource(QObject *parent)
    : QObject(parent)
{
}

CanSource::~CanSource() = default;

void CanSource::setState(State state, const QString &detail)
{
    if (m_state == state && m_statusText == detail)
        return;
    m_state = state;
    m_statusText = detail;
    emit stateChanged(m_state, m_statusText);
}
