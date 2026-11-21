#pragma once

#include <QByteArray>
#include <QVector>

#include "telemetry/telemetryvalue.h"

// Wire codec for the tagged union sent by dash-connector over BLE.
//
// Record layout (6 bytes, little-endian payload):
//   byte 0: tag
//   byte 1: type (0 float32, 1 int32, 2 uint32, 3 bool)
//   bytes 2..5: value
//
// A notification carries 1..3 complete records (6/12/18 bytes); it is never
// padded. Do not send an in-memory C struct directly (padding/alignment).
namespace telemetry {

enum class WireType : quint8 {
    Float32 = 0,
    Int32 = 1,
    UInt32 = 2,
    Bool = 3
};

inline constexpr int RecordSize = 6;

QByteArray encode(const TelemetryValue &value, WireType type = WireType::Float32);
bool decodeRecord(const QByteArray &record, TelemetryValue *out);
QVector<TelemetryValue> decodeNotification(const QByteArray &payload);

} // namespace telemetry
