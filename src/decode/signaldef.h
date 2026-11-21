#pragma once

#include <QtGlobal>

// Every value the G29 PT-CAN map can provide.
enum class SignalId : quint8 {
    Rpm,
    EngineTorque,
    GearboxTorque,
    Accelerator,
    Throttle,
    BrakePressureFront,
    BrakePercent,
    BrakePressureRear,
    Abs,
    Asc,
    BrakeStatus,
    AccelX,
    AccelY,
    YawRate,
    Speed,
    HighBeam,
    TurnLeft,
    TurnRight,
    WheelRearLeft,
    WheelRearRight,
    WheelFrontLeft,
    WheelFrontRight,
    BatteryVoltage,
    FuelRaw,
    AmbientTemp,
    Steering,
    FuelUsed,
    FuelLamp,
    FuelRange,
    CoolantTemp,
    OilTemp,
    MaxRpm,
    Gear,
    Count
};

inline constexpr int kSignalCount = static_cast<int>(SignalId::Count);

// One row of the G29 PT-CAN map.
//
// The reference table mixes units depending on "Bit Mode":
//   bitMode == true : offset/length are MSB-first bit positions (bit 0 = MSB
//                     of byte 0), and the field may straddle byte boundaries.
//   bitMode == false: offset/length are byte positions, big-endian.
//
// Value formula: value = raw * mul / div + add, with raw treated as unsigned
// unless signedRaw is set. The published bias constants (e.g. torque
// -1023.5) only make sense for unsigned raw values.
struct SignalDef
{
    SignalId id;
    const char *key;    // stable name used by QML and debug output
    const char *label;  // human readable
    const char *unit;
    quint16 canId;
    quint8 offset;
    quint8 length;
    bool bitMode;
    bool signedRaw;
    double mul;
    double div;
    double add;
    quint8 decimals;
    quint16 timeoutMs;
};
