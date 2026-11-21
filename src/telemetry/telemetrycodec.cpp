#include "telemetry/telemetrycodec.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

namespace telemetry {

namespace {

constexpr int TagOffset = 0;
constexpr int TypeOffset = 1;
constexpr int ValueOffset = 2;

quint32 readLe32(const QByteArray &data, int offset)
{
    return quint32(quint8(data.at(offset)))
        | (quint32(quint8(data.at(offset + 1))) << 8)
        | (quint32(quint8(data.at(offset + 2))) << 16)
        | (quint32(quint8(data.at(offset + 3))) << 24);
}

void appendLe32(QByteArray &out, quint32 value)
{
    out.append(char(value & 0xFFu));
    out.append(char((value >> 8) & 0xFFu));
    out.append(char((value >> 16) & 0xFFu));
    out.append(char((value >> 24) & 0xFFu));
}

} // namespace

QByteArray encode(const TelemetryValue &value, WireType type)
{
    QByteArray out;
    out.reserve(RecordSize);
    out.append(char(value.tag));
    out.append(char(type));

    quint32 bits = 0;
    switch (type) {
    case WireType::Float32:
        bits = std::bit_cast<quint32>(static_cast<float>(value.value));
        break;
    case WireType::Int32:
        bits = static_cast<quint32>(static_cast<qint32>(std::llround(value.value)));
        break;
    case WireType::UInt32:
        bits = static_cast<quint32>(std::llround(value.value));
        break;
    case WireType::Bool:
        bits = value.value != 0.0 ? 1u : 0u;
        break;
    }

    appendLe32(out, bits);
    return out;
}

bool decodeRecord(const QByteArray &record, TelemetryValue *out)
{
    if (!out || record.size() < RecordSize)
        return false;

    const quint8 tag = quint8(record.at(TagOffset));
    if (tag == 0)
        return false;

    const quint8 rawType = quint8(record.at(TypeOffset));
    if (rawType > quint8(WireType::Bool))
        return false;

    const quint32 bits = readLe32(record, ValueOffset);
    double value = 0.0;

    switch (static_cast<WireType>(rawType)) {
    case WireType::Float32:
        value = static_cast<double>(std::bit_cast<float>(bits));
        break;
    case WireType::Int32:
        value = static_cast<double>(static_cast<qint32>(bits));
        break;
    case WireType::UInt32:
        value = static_cast<double>(bits);
        break;
    case WireType::Bool:
        value = (bits & 0xFFu) != 0 ? 1.0 : 0.0;
        break;
    }

    if (!std::isfinite(value))
        return false;

    out->tag = tag;
    out->value = value;
    return true;
}

QVector<TelemetryValue> decodeNotification(const QByteArray &payload)
{
    QVector<TelemetryValue> values;
    values.reserve(payload.size() / RecordSize);

    for (int offset = 0; offset + RecordSize <= payload.size(); offset += RecordSize) {
        TelemetryValue value;
        if (decodeRecord(payload.mid(offset, RecordSize), &value))
            values.append(value);
    }

    return values;
}

} // namespace telemetry
