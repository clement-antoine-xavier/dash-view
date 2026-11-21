#pragma once

#include <QElapsedTimer>
#include <QMap>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include <array>

#include "can/canframe.h"
#include "decode/signaldef.h"

// Decoded, dashboard-facing view of the vehicle. Values are coalesced to ~30 Hz
// for QML and disappear after their per-signal timeout so the UI can show "—".
class VehicleState : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged)
    Q_PROPERTY(bool linkAlive READ linkAlive NOTIFY statsChanged)
    Q_PROPERTY(int framesPerSecond READ framesPerSecond NOTIFY statsChanged)
    Q_PROPERTY(int lastFrameAgeMs READ lastFrameAgeMs NOTIFY statsChanged)
    Q_PROPERTY(QString sourceName READ sourceName NOTIFY sourceNameChanged)
    Q_PROPERTY(QString sourceState READ sourceState NOTIFY sourceStateChanged)

public:
    explicit VehicleState(QObject *parent = nullptr);

    QVariantMap values() const { return m_values; }
    bool linkAlive() const { return m_linkAlive; }
    int framesPerSecond() const { return m_fps; }
    int lastFrameAgeMs() const { return m_lastFrameAgeMs; }

    QString sourceName() const { return m_sourceName; }
    void setSourceName(const QString &name);

    QString sourceState() const { return m_sourceState; }
    void setSourceState(const QString &state);

    Q_INVOKABLE QVariantList debugRows() const;
    Q_INVOKABLE QVariantList debugFrames() const;

public slots:
    void ingestFrame(const CanFrame &frame);
    void reset();

signals:
    void valuesChanged();
    void statsChanged();
    void sourceNameChanged();
    void sourceStateChanged();

private slots:
    void flush();
    void updateStats();

private:
    struct Entry {
        qint64 raw = 0;
        double value = 0.0;
        qint64 updatedMs = 0;
        bool valid = false;
    };

    struct FrameInfo {
        std::array<quint8, 8> data{};
        quint8 dlc = 0;
        qint64 updatedMs = 0;
        quint32 count = 0;
    };

    qint64 nowMs() const { return m_clock.elapsed(); }

    QElapsedTimer m_clock;
    QTimer m_flushTimer;
    QTimer m_statsTimer;

    std::array<Entry, kSignalCount> m_entries{};
    QMap<quint32, FrameInfo> m_frames;

    QVariantMap m_values;
    QString m_sourceName{QStringLiteral("Mock drive cycle")};
    QString m_sourceState{QStringLiteral("STOPPED")};

    quint64 m_framesSinceTick = 0;
    qint64 m_lastFrameMs = -1;
    int m_fps = 0;
    int m_lastFrameAgeMs = -1;
    bool m_linkAlive = false;
};
