#include "vehicle/vehiclestate.h"

#include <QStringList>
#include <QtGlobal>

#include <cmath>
#include <limits>

#include "decode/decoder.h"

namespace {

QString hexId(quint32 id)
{
    return QStringLiteral("0x")
        + QString::number(id, 16).toUpper().rightJustified(3, QLatin1Char('0'));
}

QString gearLabel(int gear)
{
    if (gear < 0)
        return gear == -1 ? QStringLiteral("R") : QStringLiteral("P");
    if (gear == 0)
        return QStringLiteral("N");
    return QString::number(qMin(gear, 8));
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

void VehicleState::setSourceName(const QString &name)
{
    if (m_sourceName == name)
        return;
    m_sourceName = name;
    emit sourceNameChanged();
}

void VehicleState::setSourceState(const QString &state)
{
    if (m_sourceState == state)
        return;
    m_sourceState = state;
    emit sourceStateChanged();
}

void VehicleState::ingestFrame(const CanFrame &frame)
{
    if (frame.id > 0x7FF)
        return;

    const qint64 now = nowMs();
    ++m_framesSinceTick;
    m_lastFrameMs = now;

    const auto &defs = canmap::signalsForId(frame.id);
    for (const SignalDef *def : defs) {
        qint64 raw = 0;
        if (!decoder::decodeRaw(*def, frame, &raw))
            continue;

        Entry &entry = m_entries.at(static_cast<int>(def->id));
        entry.raw = raw;
        entry.value = decoder::scale(*def, raw);
        entry.updatedMs = now;
        entry.valid = true;
    }

    FrameInfo &info = m_frames[frame.id];
    info.data = frame.data;
    info.dlc = frame.dlc;
    info.updatedMs = now;
    ++info.count;
}

void VehicleState::flush()
{
    const qint64 now = nowMs();
    QVariantMap next;

    for (const SignalDef &def : canmap::allSignals()) {
        const Entry &entry = m_entries.at(static_cast<int>(def.id));
        if (!entry.valid)
            continue;
        if (now - entry.updatedMs > def.timeoutMs)
            continue;

        // The Max RPM row is suspect (see docs/signals.md); drop implausible
        // values so QML falls back to its 7000 rpm shift-light reference.
        if (def.id == SignalId::MaxRpm && (entry.value < 5000.0 || entry.value > 9000.0))
            continue;

        next.insert(QString::fromLatin1(def.key), entry.value);
    }

    const auto gearIt = next.constFind(QStringLiteral("gear"));
    if (gearIt != next.constEnd())
        next.insert(QStringLiteral("gearLabel"), gearLabel(static_cast<int>(std::lround(gearIt.value().toDouble()))));

    if (next != m_values) {
        m_values = std::move(next);
        emit valuesChanged();
    }
}

void VehicleState::updateStats()
{
    const qint64 now = nowMs();

    m_fps = static_cast<int>(std::lround(static_cast<double>(m_framesSinceTick) * 2.0));
    m_framesSinceTick = 0;

    m_lastFrameAgeMs = m_lastFrameMs >= 0
        ? static_cast<int>(qMin<qint64>(now - m_lastFrameMs, std::numeric_limits<int>::max()))
        : -1;

    m_linkAlive = m_lastFrameMs >= 0 && (now - m_lastFrameMs) < 1500;

    emit statsChanged();
}

void VehicleState::reset()
{
    m_entries.fill(Entry{});
    m_frames.clear();
    m_values.clear();
    m_framesSinceTick = 0;
    m_lastFrameMs = -1;
    m_fps = 0;
    m_lastFrameAgeMs = -1;
    m_linkAlive = false;
    emit valuesChanged();
    emit statsChanged();
}

QVariantList VehicleState::debugRows() const
{
    QVariantList rows;
    const qint64 now = nowMs();

    for (const SignalDef &def : canmap::allSignals()) {
        const Entry &entry = m_entries.at(static_cast<int>(def.id));
        const int bits = decoder::rawBits(def);
        const quint64 mask = bits >= 64 ? ~quint64(0) : ((quint64(1) << bits) - 1);
        const quint64 rawMasked = static_cast<quint64>(entry.raw) & mask;

        QVariantMap row;
        row.insert(QStringLiteral("key"), QString::fromLatin1(def.key));
        row.insert(QStringLiteral("id"), hexId(def.canId));
        row.insert(QStringLiteral("label"), QString::fromUtf8(def.label));
        row.insert(QStringLiteral("unit"), QString::fromUtf8(def.unit));
        row.insert(QStringLiteral("raw"), entry.valid ? QVariant::fromValue<qlonglong>(entry.raw) : QVariant());
        row.insert(QStringLiteral("rawHex"), entry.valid
                       ? QStringLiteral("0x") + QString::number(rawMasked, 16).toUpper()
                       : QString());
        row.insert(QStringLiteral("value"), entry.valid ? QVariant(entry.value) : QVariant());
        row.insert(QStringLiteral("fresh"), entry.valid && (now - entry.updatedMs) <= def.timeoutMs);
        row.insert(QStringLiteral("ageMs"), entry.valid ? static_cast<int>(now - entry.updatedMs) : -1);
        row.insert(QStringLiteral("at"), QStringLiteral("%1%2+%3")
                       .arg(def.bitMode ? QStringLiteral("b") : QStringLiteral("B"))
                       .arg(def.offset)
                       .arg(def.length));
        rows.append(row);
    }

    return rows;
}

QVariantList VehicleState::debugFrames() const
{
    QVariantList rows;
    const qint64 now = nowMs();

    for (auto it = m_frames.cbegin(); it != m_frames.cend(); ++it) {
        QStringList bytes;
        const int count = qMin<int>(it.value().dlc, 8);
        for (int i = 0; i < count; ++i)
            bytes << QString::number(it.value().data.at(i), 16).toUpper().rightJustified(2, QLatin1Char('0'));

        QVariantMap row;
        row.insert(QStringLiteral("id"), hexId(it.key()));
        row.insert(QStringLiteral("dlc"), it.value().dlc);
        row.insert(QStringLiteral("payload"), bytes.join(QLatin1Char(' ')));
        row.insert(QStringLiteral("ageMs"), static_cast<int>(now - it.value().updatedMs));
        row.insert(QStringLiteral("count"), it.value().count);
        rows.append(row);
    }

    return rows;
}
