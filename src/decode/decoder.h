#pragma once

#include <QVector>

#include "can/canframe.h"
#include "decode/signaldef.h"

namespace canmap {

// All rows of the G29/J29 PT-CAN map, in SignalId order.
const QVector<SignalDef> &allSignals();

const SignalDef *definition(SignalId id);

// All signals carried by one CAN identifier (frames usually carry 1-4).
const QVector<const SignalDef *> &signalsForId(quint32 canId);

} // namespace canmap

namespace decoder {

int rawBits(const SignalDef &def);

bool decodeRaw(const SignalDef &def, const CanFrame &frame, qint64 *raw);
bool decode(const SignalDef &def, const CanFrame &frame, double *value);

// Inverse of the value formula, clamped to the field width. Used by the mock
// source and by round-trip unit tests.
quint64 encodeRaw(const SignalDef &def, double value);

double scale(const SignalDef &def, qint64 raw);

} // namespace decoder
