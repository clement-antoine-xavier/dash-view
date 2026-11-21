#include "telemetry/signaltag.h"

#include <QHash>

namespace {

using sig::SignalInfo;

SignalInfo makeInfo(sig::Tag tag,
                     const char *key,
                     const char *label,
                     const char *unit,
                     int decimals,
                     quint16 timeoutMs,
                     double minValue,
                     double maxValue,
                     bool boolean = false)
{
    SignalInfo info;
    info.tag = tag;
    info.key = key;
    info.label = label;
    info.unit = unit;
    info.decimals = decimals;
    info.timeoutMs = timeoutMs;
    info.minValue = minValue;
    info.maxValue = maxValue;
    info.boolean = boolean;
    info.known = true;
    return info;
}

const QVector<SignalInfo> &table()
{
    static const QVector<SignalInfo> rows = {
        makeInfo(sig::Rpm, "rpm", "RPM", "rpm", 0, 1000, 0, 12000),
        makeInfo(sig::Speed, "speed", "Speed", "km/h", 0, 1000, 0, 400),
        makeInfo(sig::Gear, "gear", "Gear", "", 0, 2000, -2, 12),
        makeInfo(sig::Throttle, "throttle", "Throttle", "%", 1, 1000, 0, 100),
        makeInfo(sig::Accelerator, "accelerator", "Accelerator", "%", 1, 1000, 0, 100),
        makeInfo(sig::Brake, "brakePct", "Brake", "%", 1, 1000, 0, 100),
        makeInfo(sig::Steering, "steering", "Steering", "°", 1, 1000, -720, 720),
        makeInfo(sig::EngineTorque, "engineTorque", "Engine torque", "Nm", 0, 1000, -1000, 2000),
        makeInfo(sig::GearboxTorque, "gearboxTorque", "Gearbox torque", "Nm", 0, 1000, -1000, 2000),
        makeInfo(sig::CoolantTemp, "coolantTemp", "Coolant", "°C", 0, 5000, -40, 150),
        makeInfo(sig::OilTemp, "oilTemp", "Oil", "°C", 0, 5000, -40, 180),
        makeInfo(sig::AmbientTemp, "ambientTemp", "Ambient", "°C", 1, 10000, -40, 60),
        makeInfo(sig::BatteryVoltage, "batteryVoltage", "Battery", "V", 1, 5000, 0, 20),
        makeInfo(sig::FuelRange, "fuelRange", "Fuel range", "km", 0, 5000, 0, 1500),
        makeInfo(sig::FuelUsed, "fuelUsed", "Fuel used", "L", 1, 5000, 0, 2000),
        makeInfo(sig::FuelLevel, "fuelLevel", "Fuel level", "%", 0, 5000, 0, 100),
        makeInfo(sig::FuelLamp, "fuelLamp", "Fuel lamp", "", 0, 5000, 0, 1, true),
        makeInfo(sig::MaxRpm, "maxRpm", "Max RPM", "rpm", 0, 5000, 0, 12000),
        makeInfo(sig::BrakePressureFront, "brakePressureFront", "Brake pressure F", "bar", 1, 1000, 0, 300),
        makeInfo(sig::BrakePressureRear, "brakePressureRear", "Brake pressure R", "bar", 1, 1000, 0, 300),
        makeInfo(sig::YawRate, "yawRate", "Yaw rate", "°/s", 1, 1000, -200, 200),
        makeInfo(sig::AccelX, "accelX", "Accel X", "g", 2, 1000, -3, 3),
        makeInfo(sig::AccelY, "accelY", "Accel Y", "g", 2, 1000, -3, 3),
        makeInfo(sig::WheelFrontLeft, "wheelFrontLeft", "Wheel FL", "km/h", 1, 1000, 0, 400),
        makeInfo(sig::WheelFrontRight, "wheelFrontRight", "Wheel FR", "km/h", 1, 1000, 0, 400),
        makeInfo(sig::WheelRearLeft, "wheelRearLeft", "Wheel RL", "km/h", 1, 1000, 0, 400),
        makeInfo(sig::WheelRearRight, "wheelRearRight", "Wheel RR", "km/h", 1, 1000, 0, 400),
        makeInfo(sig::HighBeam, "highBeam", "High beam", "", 0, 2000, 0, 1, true),
        makeInfo(sig::TurnLeft, "turnLeft", "Turn left", "", 0, 2000, 0, 1, true),
        makeInfo(sig::TurnRight, "turnRight", "Turn right", "", 0, 2000, 0, 1, true),
        makeInfo(sig::Abs, "abs", "ABS", "", 0, 2000, 0, 1, true),
        makeInfo(sig::Asc, "asc", "ASC", "", 0, 2000, 0, 1, true),
        makeInfo(sig::BrakeStatus, "brakeStatus", "Brake status", "#", 0, 2000, 0, 10),
        makeInfo(sig::EngineLoad, "engineLoad", "Engine load", "%", 0, 2000, 0, 100),
        makeInfo(sig::IntakeTemp, "intakeTemp", "Intake temp", "°C", 0, 5000, -40, 120),
        makeInfo(sig::Boost, "boost", "Boost", "bar", 2, 1000, -1, 5),
        makeInfo(sig::OilPressure, "oilPressure", "Oil pressure", "bar", 1, 2000, 0, 12),
        makeInfo(sig::FuelPressure, "fuelPressure", "Fuel pressure", "bar", 1, 2000, 0, 12),
        makeInfo(sig::Lambda, "lambda", "Lambda", "λ", 2, 1000, 0.5, 1.5),
        makeInfo(sig::Egt, "egt", "EGT", "°C", 0, 2000, 0, 1200),
        makeInfo(sig::Odometer, "odometer", "Odometer", "km", 0, 10000, 0, 2000000),
        makeInfo(sig::BatterySoc, "batterySoc", "Battery SOC", "%", 1, 5000, 0, 100),
        makeInfo(sig::PowerKw, "powerKw", "Power", "kW", 1, 1000, -500, 2000),
        makeInfo(sig::MotorTemp, "motorTemp", "Motor temp", "°C", 0, 5000, -40, 250),
    };

    Q_ASSERT(rows.size() == 44);
    return rows;
}

const QHash<sig::Tag, SignalInfo> &index()
{
    static const QHash<sig::Tag, SignalInfo> byTag = [] {
        QHash<sig::Tag, SignalInfo> map;
        for (const SignalInfo &info : table())
            map.insert(info.tag, info);
        return map;
    }();
    return byTag;
}

} // namespace

namespace sig {

const QVector<SignalInfo> &catalog()
{
    return table();
}

SignalInfo info(Tag tag)
{
    const auto it = index().constFind(tag);
    if (it != index().constEnd())
        return it.value();

    SignalInfo custom;
    custom.tag = tag;
    custom.key = nullptr;
    custom.label = "";
    custom.unit = "";
    custom.decimals = 2;
    custom.timeoutMs = 5000;
    custom.boolean = false;
    custom.known = false;
    return custom;
}

QString key(Tag tag)
{
    const SignalInfo i = info(tag);
    return i.key ? QString::fromLatin1(i.key) : QString();
}

QString label(Tag tag)
{
    const SignalInfo i = info(tag);
    if (i.known)
        return QString::fromUtf8(i.label);
    return QStringLiteral("Custom 0x")
        + QString::number(tag, 16).toUpper().rightJustified(2, QLatin1Char('0'));
}

QString unit(Tag tag)
{
    return QString::fromUtf8(info(tag).unit);
}

QString hex(Tag tag)
{
    return QStringLiteral("0x")
        + QString::number(tag, 16).toUpper().rightJustified(2, QLatin1Char('0'));
}

QString logName(Tag tag)
{
    const QString k = key(tag);
    if (!k.isEmpty())
        return k;
    return QStringLiteral("custom_")
        + QString::number(tag, 16).toUpper().rightJustified(2, QLatin1Char('0'));
}

bool isKnown(Tag tag)
{
    return info(tag).known;
}

quint16 timeoutMs(Tag tag)
{
    return info(tag).timeoutMs;
}

int decimals(Tag tag)
{
    return info(tag).decimals;
}

} // namespace sig
