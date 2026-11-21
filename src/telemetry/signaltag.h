#pragma once

#include <QString>
#include <QVector>
#include <QtGlobal>

// Vehicle-agnostic telemetry tags. dash-connector decodes the vehicle's CAN
// bus and sends values using these tags; Dash View never sees raw CAN.
namespace sig {

using Tag = quint8;

inline constexpr Tag Invalid = 0x00;
inline constexpr Tag Rpm = 0x01;
inline constexpr Tag Speed = 0x02;
inline constexpr Tag Gear = 0x03;
inline constexpr Tag Throttle = 0x04;
inline constexpr Tag Accelerator = 0x05;
inline constexpr Tag Brake = 0x06;
inline constexpr Tag Steering = 0x07;
inline constexpr Tag EngineTorque = 0x08;
inline constexpr Tag GearboxTorque = 0x09;
inline constexpr Tag CoolantTemp = 0x0A;
inline constexpr Tag OilTemp = 0x0B;
inline constexpr Tag AmbientTemp = 0x0C;
inline constexpr Tag BatteryVoltage = 0x0D;
inline constexpr Tag FuelRange = 0x0E;
inline constexpr Tag FuelUsed = 0x0F;
inline constexpr Tag FuelLevel = 0x10;
inline constexpr Tag FuelLamp = 0x11;
inline constexpr Tag MaxRpm = 0x12;
inline constexpr Tag BrakePressureFront = 0x13;
inline constexpr Tag BrakePressureRear = 0x14;
inline constexpr Tag YawRate = 0x15;
inline constexpr Tag AccelX = 0x16;
inline constexpr Tag AccelY = 0x17;
inline constexpr Tag WheelFrontLeft = 0x18;
inline constexpr Tag WheelFrontRight = 0x19;
inline constexpr Tag WheelRearLeft = 0x1A;
inline constexpr Tag WheelRearRight = 0x1B;
inline constexpr Tag HighBeam = 0x1C;
inline constexpr Tag TurnLeft = 0x1D;
inline constexpr Tag TurnRight = 0x1E;
inline constexpr Tag Abs = 0x1F;
inline constexpr Tag Asc = 0x20;
inline constexpr Tag BrakeStatus = 0x21;
inline constexpr Tag EngineLoad = 0x22;
inline constexpr Tag IntakeTemp = 0x23;
inline constexpr Tag Boost = 0x24;
inline constexpr Tag OilPressure = 0x25;
inline constexpr Tag FuelPressure = 0x26;
inline constexpr Tag Lambda = 0x27;
inline constexpr Tag Egt = 0x28;
inline constexpr Tag Odometer = 0x29;
inline constexpr Tag BatterySoc = 0x2A;
inline constexpr Tag PowerKw = 0x2B;
inline constexpr Tag MotorTemp = 0x2C;

// 0x2D..0x7F: reserved for future standard tags.
inline constexpr Tag CustomBase = 0x80; // 0x80..0xEF: vehicle-specific
// 0xF0..0xFF: reserved for protocol/meta use.

struct SignalInfo
{
    Tag tag = Invalid;
    const char *key = nullptr;   // stable QML/log key (known tags only)
    const char *label = "";
    const char *unit = "";
    int decimals = 1;
    quint16 timeoutMs = 5000;
    double minValue = 0.0;
    double maxValue = 0.0;
    bool boolean = false;
    bool known = false;
};

const QVector<SignalInfo> &catalog();

// Known tags return their catalog entry; custom tags return a synthetic entry.
SignalInfo info(Tag tag);

QString key(Tag tag);
QString label(Tag tag);
QString unit(Tag tag);
QString hex(Tag tag);       // "0x01"
QString logName(Tag tag);   // key, or "custom_80"
bool isKnown(Tag tag);
quint16 timeoutMs(Tag tag);
int decimals(Tag tag);

} // namespace sig
