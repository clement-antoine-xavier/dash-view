#pragma once

#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include "telemetry/signaltag.h"
#include "telemetry/telemetryvalue.h"

// Tagged values from any vehicle, coalesced to ~30 Hz for QML. Values vanish
// after their per-tag timeout so the dashboard shows "—" for missing data.
class VehicleState : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged)
    Q_PROPERTY(QString vehicleName READ vehicleName WRITE setVehicleName NOTIFY vehicleNameChanged)
    Q_PROPERTY(QString sourceName READ sourceName WRITE setSourceName NOTIFY sourceNameChanged)
    Q_PROPERTY(QString sourceLabel READ sourceLabel WRITE setSourceLabel NOTIFY sourceLabelChanged)
    Q_PROPERTY(QString sourceState READ sourceState WRITE setSourceState NOTIFY sourceStateChanged)
    Q_PROPERTY(bool linkAlive READ linkAlive NOTIFY statsChanged)
    Q_PROPERTY(int valuesPerSecond READ valuesPerSecond NOTIFY statsChanged)
    Q_PROPERTY(int lastValueAgeMs READ lastValueAgeMs NOTIFY statsChanged)

public:
    explicit VehicleState(QObject *parent = nullptr);

    QVariantMap values() const { return m_values; }

    QString vehicleName() const { return m_vehicleName; }
    void setVehicleName(const QString &name);

    QString sourceName() const { return m_sourceName; }
    void setSourceName(const QString &name);

    QString sourceLabel() const { return m_sourceLabel; }
    void setSourceLabel(const QString &label);

    QString sourceState() const { return m_sourceState; }
    void setSourceState(const QString &state);

    bool linkAlive() const { return m_linkAlive; }
    int valuesPerSecond() const { return m_valuesPerSecond; }
    int lastValueAgeMs() const { return m_lastValueAgeMs; }

    Q_INVOKABLE QVariantList debugRows() const;
    Q_INVOKABLE QVariantList debugRecent() const;

public slots:
    void ingest(const TelemetryValue &value);
    void reset();

signals:
    void valuesChanged();
    void vehicleNameChanged();
    void sourceNameChanged();
    void sourceLabelChanged();
    void sourceStateChanged();
    void statsChanged();

private slots:
    void flush();
    void updateStats();

private:
    struct Entry
    {
        double value = 0.0;
        qint64 updatedMs = 0;
        quint32 count = 0;
    };

    struct Recent
    {
        quint8 tag = 0;
        double value = 0.0;
        qint64 at = 0;
    };

    qint64 nowMs() const { return m_clock.elapsed(); }

    QElapsedTimer m_clock;
    QTimer m_flushTimer;
    QTimer m_statsTimer;

    QHash<quint8, Entry> m_entries;
    QVector<Recent> m_recent;
    QVariantMap m_values;

    QString m_vehicleName{QStringLiteral("Vehicle")};
    QString m_sourceName{QStringLiteral("None")};
    QString m_sourceLabel{QStringLiteral("—")};
    QString m_sourceState{QStringLiteral("STOPPED")};

    quint64 m_valuesSinceTick = 0;
    qint64 m_lastValueMs = -1;
    int m_valuesPerSecond = 0;
    int m_lastValueAgeMs = -1;
    bool m_linkAlive = false;
};
