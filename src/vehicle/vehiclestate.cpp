#include "vehicle/vehiclestate.h"

#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

QString gearLabel(int gear)
{
    if (gear < 0)
        return gear == -1 ? QStringLiteral("R") : QStringLiteral("P");
    if (gear == 0)
        return QStringLiteral("N");
    return QString::number(gear);
}

} // namespace

VehicleState::VehicleState(QObject *parent)
    : QObject(parent)
{
    m_clock.start();

    m_flushTimer.setInterval(33);
    connect(&m_flushTimer, &QTimer::timeout, this, &VehicleState::flush);
    m_flushTimer.start();

    m_statsTimer.setInterval(500);
    connect(&m_statsTimer, &QTimer::timeout, this, &VehicleState::updateStats);
    m_statsTimer.start();
}

void VehicleState::setVehicleName(const QString &name)
{
    const QString cleaned = name.trimmed();
    if (cleaned.isEmpty() || m_vehicleName == cleaned)
        return;
    m_vehicleName = cleaned;
    emit vehicleNameChanged();
}

void VehicleState::setSourceName(const QString &name)
{
    if (m_sourceName == name)
        return;
    m_sourceName = name;
    emit sourceNameChanged();
}

void VehicleState::setSourceLabel(const QString &label)
{
    if (m_sourceLabel == label)
        return;
    m_sourceLabel = label;
    emit sourceLabelChanged();
}

void VehicleState::setSourceState(const QString &state)
{
    if (m_sourceState == state)
        return;
    m_sourceState = state;
    emit sourceStateChanged();
}

void VehicleState::ingest(const TelemetryValue &value)
{
    if (value.tag == 0 || !std::isfinite(value.value))
        return;

    const qint64 now = nowMs();
    ++m_valuesSinceTick;
    m_lastValueMs = now;

    Entry &entry = m_entries[value.tag];
    entry.value = value.value;
    entry.updatedMs = now;
    ++entry.count;

    m_recent.append(Recent{value.tag, value.value, now});
    if (m_recent.size() > 120)
        m_recent.remove(0, m_recent.size() - 120);
}

void VehicleState::flush()
{
    const qint64 now = nowMs();
    QVariantMap next;

    for (const sig::SignalInfo &info : sig::catalog()) {
        const auto it = m_entries.constFind(info.tag);
        if (it == m_entries.constEnd())
            continue;
        if (now - it.value().updatedMs > info.timeoutMs)
            continue;
        if (!info.key)
            continue;
        next.insert(QString::fromLatin1(info.key), it.value().value);
    }

    const auto gearIt = next.constFind(QStringLiteral("gear"));
    if (gearIt != next.constEnd()) {
        const int gear = static_cast<int>(std::lround(gearIt.value().toDouble()));
        next.insert(QStringLiteral("gearLabel"), gearLabel(gear));
    }

    if (next != m_values) {
        m_values = std::move(next);
        emit valuesChanged();
    }
}

void VehicleState::updateStats()
{
    const qint64 now = nowMs();

    m_valuesPerSecond = static_cast<int>(std::lround(static_cast<double>(m_valuesSinceTick) * 2.0));
    m_valuesSinceTick = 0;

    m_lastValueAgeMs = m_lastValueMs >= 0
        ? static_cast<int>(qMin<qint64>(now - m_lastValueMs, std::numeric_limits<int>::max()))
        : -1;

    m_linkAlive = m_lastValueMs >= 0 && (now - m_lastValueMs) < 1500;

    emit statsChanged();
}

void VehicleState::reset()
{
    m_entries.clear();
    m_recent.clear();
    m_values.clear();
    m_valuesSinceTick = 0;
    m_lastValueMs = -1;
    m_valuesPerSecond = 0;
    m_lastValueAgeMs = -1;
    m_linkAlive = false;
    emit valuesChanged();
    emit statsChanged();
}

QVariantList VehicleState::debugRows() const
{
    QVector<quint8> tags;
    tags.reserve(sig::catalog().size() + m_entries.size());
    for (const sig::SignalInfo &info : sig::catalog())
        tags.append(info.tag);
    for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
        if (!sig::isKnown(it.key()))
            tags.append(it.key());
    }
    std::sort(tags.begin(), tags.end());
    tags.erase(std::unique(tags.begin(), tags.end()), tags.end());

    const qint64 now = nowMs();
    QVariantList rows;

    for (quint8 tag : tags) {
        const sig::SignalInfo info = sig::info(tag);
        const auto it = m_entries.constFind(tag);
        const bool present = it != m_entries.constEnd();

        QVariantMap row;
        row.insert(QStringLiteral("tag"), sig::hex(tag));
        row.insert(QStringLiteral("key"), info.key ? QString::fromLatin1(info.key) : QString());
        row.insert(QStringLiteral("label"), sig::label(tag));
        row.insert(QStringLiteral("unit"), QString::fromUtf8(info.unit));
        row.insert(QStringLiteral("decimals"), info.decimals);
        row.insert(QStringLiteral("value"), present ? QVariant(it.value().value) : QVariant());
        row.insert(QStringLiteral("ageMs"), present ? static_cast<int>(now - it.value().updatedMs) : -1);
        row.insert(QStringLiteral("count"), present ? it.value().count : 0);
        row.insert(QStringLiteral("fresh"), present && (now - it.value().updatedMs <= info.timeoutMs));
        row.insert(QStringLiteral("known"), info.known);
        rows.append(row);
    }

    return rows;
}

QVariantList VehicleState::debugRecent() const
{
    const qint64 now = nowMs();
    QVariantList rows;
    rows.reserve(m_recent.size());

    for (int i = m_recent.size() - 1; i >= 0; --i) {
        const Recent &recent = m_recent.at(i);
        QVariantMap row;
        row.insert(QStringLiteral("tag"), sig::hex(recent.tag));
        row.insert(QStringLiteral("name"), sig::logName(recent.tag));
        row.insert(QStringLiteral("label"), sig::label(recent.tag));
        row.insert(QStringLiteral("value"), recent.value);
        row.insert(QStringLiteral("ageMs"), static_cast<int>(now - recent.at));
        rows.append(row);
    }

    return rows;
}
