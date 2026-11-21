#include "decode/decoder.h"

#include <QHash>
#include <QtGlobal>

namespace {

SignalDef makeDef(SignalId id,
                  const char *key,
                  const char *label,
                  const char *unit,
                  quint16 canId,
                  quint8 offset,
                  quint8 length,
                  bool bitMode,
                  double mul,
                  double div,
                  double add,
                  quint8 decimals,
                  quint16 timeoutMs,
                  bool signedRaw = false)
{
    return SignalDef{id, key, label, unit, canId, offset, length, bitMode, signedRaw,
                     mul, div, add, decimals, timeoutMs};
}

const QVector<SignalDef> &table()
{
    static const QVector<SignalDef> rows = [] {
        QVector<SignalDef> v;
        v.reserve(kSignalCount);

        //            id                     key                 label              unit     id     off len bit   mul  div       add       dec  timeout
        v << makeDef(SignalId::Rpm,              "rpm",              "RPM",               "rpm",  0x0A5,  5, 2, false, 1,   4,        0,        0,  1000);
        v << makeDef(SignalId::EngineTorque,     "engineTorque",     "Engine torque",     "Nm",   0x0A5, 16,12, true,  1,   2,        -1023.5,  0,  1000);
        v << makeDef(SignalId::GearboxTorque,    "gearboxTorque",    "Gearbox torque",    "Nm",   0x0A5, 28,12, true,  1,   2,        -1023.5,  0,  1000);
        v << makeDef(SignalId::Accelerator,      "accelerator",      "Accelerator",       "%",    0x0D9, 16,12, true,  1,   40,       0,        1,  1000);
        v << makeDef(SignalId::Throttle,         "throttle",         "Throttle",          "%",    0x0D9, 32,12, true,  1,   40,       0,        1,  1000);
        v << makeDef(SignalId::BrakePressureFront,"brakePressureFront","Brake pressure F", "bar",  0x0EF,  2, 2, false, -3,  200,      480,      1,  1000);
        v << makeDef(SignalId::BrakePercent,     "brakePct",         "Brake",             "%",    0x0EF,  3, 1, false, -2,  1,        250,      0,  1000);
        v << makeDef(SignalId::BrakePressureRear, "brakePressureRear","Brake pressure R", "bar",  0x0EF,  5, 2, false, -3,  200,      480,      1,  1000);
        v << makeDef(SignalId::Abs,              "abs",              "ABS",               "#",    0x173, 28,12, true,  1,   1,        0,        0,  2000);
        v << makeDef(SignalId::Asc,              "asc",              "ASC",               "#",    0x173, 40,12, true,  1,   1,        0,        0,  2000);
        v << makeDef(SignalId::BrakeStatus,      "brakeStatus",      "Brake status",      "#",    0x173, 56, 6, true,  1,   1,        0,        0,  2000);
        v << makeDef(SignalId::AccelX,           "accelX",           "Accel X",           "g",    0x199,  2, 2, false, -1,  5000,     6.6259,   2,  1000);
        v << makeDef(SignalId::AccelY,           "accelY",           "Accel Y",           "g",    0x19A,  2, 2, false, -1,  5000,     6.6259,   2,  1000);
        v << makeDef(SignalId::YawRate,          "yawRate",          "Yaw rate",          "°/s",  0x19F,  2, 2, false, -1,  200,      163.84,   1,  1000);
        v << makeDef(SignalId::Speed,            "speed",            "Speed",             "km/h", 0x1A1,  2, 2, false, 1,   64,       0,        0,  1000);
        v << makeDef(SignalId::HighBeam,         "highBeam",         "High beam",         "",     0x1EE,  5, 1, true,  1,   1,        0,        0,  2000);
        v << makeDef(SignalId::TurnLeft,         "turnLeft",         "Turn left",         "",     0x1F6,  4, 1, true,  1,   1,        0,        0,  2000);
        v << makeDef(SignalId::TurnRight,        "turnRight",        "Turn right",        "",     0x1F6,  5, 1, true,  1,   1,        0,        0,  2000);
        v << makeDef(SignalId::WheelRearLeft,    "wheelRearLeft",    "Wheel RL",          "km/h", 0x254,  0, 2, false, 1,   64,       -511.98001, 1, 1000);
        v << makeDef(SignalId::WheelRearRight,   "wheelRearRight",   "Wheel RR",          "km/h", 0x254,  2, 2, false, 1,   64,       -511.98001, 1, 1000);
        v << makeDef(SignalId::WheelFrontLeft,   "wheelFrontLeft",   "Wheel FL",          "km/h", 0x254,  4, 2, false, 1,   64,       -511.98001, 1, 1000);
        v << makeDef(SignalId::WheelFrontRight,  "wheelFrontRight",  "Wheel FR",          "km/h", 0x254,  6, 2, false, 1,   64,       -511.98001, 1, 1000);
        v << makeDef(SignalId::BatteryVoltage,   "batteryVoltage",   "Battery",           "V",     0x281,  1,12, true,  15,  1000,     0,        1,  5000);
        v << makeDef(SignalId::FuelRaw,          "fuelRaw",          "Fuel raw",          "#",     0x2C4,  0, 2, false, 1,   1,        0,        0,  5000);
        v << makeDef(SignalId::AmbientTemp,      "ambientTemp",      "Ambient",           "°C",    0x2CA,  0, 1, false, 1,   2,        -40,      1, 10000);
        v << makeDef(SignalId::Steering,         "steering",         "Steering",          "°",     0x301,  2, 2, false, -1,  22.75,    1440,     1,  1000);
        v << makeDef(SignalId::FuelUsed,         "fuelUsed",         "Fuel used",         "L",     0x330,  4, 1, false, 1,   1,        0,        1,  5000);
        v << makeDef(SignalId::FuelLamp,         "fuelLamp",         "Fuel lamp",         "",      0x330, 40, 1, true,  1,   1,        0,        0,  5000);
        v << makeDef(SignalId::FuelRange,        "fuelRange",        "Fuel range",        "km",    0x330, 48,12, true,  1,   16,       0,        0,  5000);
        v << makeDef(SignalId::CoolantTemp,      "coolantTemp",      "Coolant",           "°C",    0x3F9,  4, 1, false, 1,   1,        -48,      0,  5000);
        v << makeDef(SignalId::OilTemp,          "oilTemp",          "Oil",               "°C",    0x3F9,  5, 1, false, 1,   1,        -48,      0,  5000);

        // The reference page lists "multiply 1 / divide 1 / add 50" for Max RPM,
        // which turns the published sample byte 0xA3 into 213 rpm. Interpreting
        // it as raw * 50 gives 8150 rpm, the only plausible reading. This row is
        // flagged for verification against a real log (see docs/signals.md).
        v << makeDef(SignalId::MaxRpm,           "maxRpm",           "Max RPM",           "rpm",   0x3F9,  7, 1, false, 50,  1,        0,        0,  5000);

        // Gear = raw - 4 per the reference. Unsigned raw matches the published
        // sample; signedRaw is kept available in case a real log says otherwise.
        v << makeDef(SignalId::Gear,             "gear",             "Gear",              "",      0x3F9, 48, 4, true,  1,   1,        -4,       0,  2000);

        for (int i = 0; i < v.size(); ++i)
            Q_ASSERT(static_cast<int>(v.at(i).id) == i);
        Q_ASSERT(v.size() == kSignalCount);
        return v;
    }();

    return rows;
}

const QHash<quint32, QVector<const SignalDef *>> &index()
{
    static const QHash<quint32, QVector<const SignalDef *>> byId = [] {
        QHash<quint32, QVector<const SignalDef *>> map;
        for (const SignalDef &def : table())
            map[def.canId].append(&def);
        return map;
    }();
    return byId;
}

} // namespace

namespace canmap {

const QVector<SignalDef> &allSignals()
{
    return table();
}

const SignalDef *definition(SignalId id)
{
    const int idx = static_cast<int>(id);
    if (idx < 0 || idx >= kSignalCount)
        return nullptr;
    return &table().at(idx);
}

const QVector<const SignalDef *> &signalsForId(quint32 canId)
{
    static const QVector<const SignalDef *> empty;
    const auto it = index().constFind(canId);
    return it == index().constEnd() ? empty : it.value();
}

} // namespace canmap
