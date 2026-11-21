#include "decode/decoder.h"

#include <QtGlobal>

#include <cmath>

namespace decoder {

int rawBits(const SignalDef &def)
{
    return def.bitMode ? def.length : def.length * 8;
}

bool decodeRaw(const SignalDef &def, const CanFrame &frame, qint64 *raw)
{
    if (!raw)
        return false;

    const int payloadBytes = qBound(0, static_cast<int>(frame.dlc), 8);

    if (def.bitMode) {
        const int bits = def.length;
        if (bits <= 0 || bits > 63)
            return false;
        if (def.offset + bits > 64)
            return false;
        if (def.offset + bits > payloadBytes * 8)
            return false;

        quint64 value = 0;
        for (int i = 0; i < bits; ++i) {
            const int bit = def.offset + i;
            value = (value << 1) | ((frame.data[bit / 8] >> (7 - (bit % 8))) & 1u);
        }

        if (def.signedRaw) {
            const quint64 sign = quint64(1) << (bits - 1);
            if (value & sign)
                value |= ~((quint64(1) << bits) - 1);
        }

        *raw = static_cast<qint64>(value);
        return true;
    }

    if (def.length == 0 || def.length > 8)
        return false;
    if (def.offset + def.length > 8)
        return false;
    if (def.offset + def.length > payloadBytes)
        return false;

    quint64 value = 0;
    for (int i = 0; i < def.length; ++i)
        value = (value << 8) | frame.data[def.offset + i];

    if (def.signedRaw && def.length < 8) {
        const int bits = def.length * 8;
        const quint64 sign = quint64(1) << (bits - 1);
        if (value & sign)
            value |= ~((quint64(1) << bits) - 1);
    }

    *raw = static_cast<qint64>(value);
    return true;
}

double scale(const SignalDef &def, qint64 raw)
{
    if (def.div == 0.0)
        return def.add;
    return static_cast<double>(raw) * def.mul / def.div + def.add;
}

bool decode(const SignalDef &def, const CanFrame &frame, double *value)
{
    if (!value)
        return false;

    qint64 raw = 0;
    if (!decodeRaw(def, frame, &raw))
        return false;

    *value = scale(def, raw);
    return true;
}

quint64 encodeRaw(const SignalDef &def, double value)
{
    if (!std::isfinite(value) || def.mul == 0.0 || def.div == 0.0)
        return 0;

    const int bits = rawBits(def);
    if (bits <= 0 || bits > 62)
        return 0;

    const double rawValue = (value - def.add) * def.div / def.mul;
    qint64 raw = static_cast<qint64>(std::llround(rawValue));

    if (def.signedRaw) {
        const qint64 lo = -(qint64(1) << (bits - 1));
        const qint64 hi = (qint64(1) << (bits - 1)) - 1;
        raw = qBound(lo, raw, hi);
        return static_cast<quint64>(raw) & ((quint64(1) << bits) - 1);
    }

    const qint64 maxRaw = (qint64(1) << bits) - 1;
    raw = qBound<qint64>(0, raw, maxRaw);
    return static_cast<quint64>(raw);
}

} // namespace decoder
