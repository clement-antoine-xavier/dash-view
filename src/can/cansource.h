#pragma once

#include <QObject>
#include <QString>

#include "can/canframe.h"

// Abstract producer of CAN frames. The dashboard does not care whether the
// frames come from BLE, a log file or the synthetic demo cycle.
class CanSource : public QObject
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

    explicit CanSource(QObject *parent = nullptr);
    ~CanSource() override;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual QString name() const = 0;
    virtual QString description() const { return {}; }

    State state() const { return m_state; }
    QString statusText() const { return m_statusText; }

signals:
    void frameReceived(const CanFrame &frame);
    void stateChanged(CanSource::State state, const QString &detail);

protected:
    void setState(State state, const QString &detail = {});

private:
    State m_state = State::Stopped;
    QString m_statusText;
};
