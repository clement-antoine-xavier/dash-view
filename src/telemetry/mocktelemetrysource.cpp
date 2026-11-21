#include "telemetry/mocktelemetrysource.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kCycleSeconds = 80.0;

struct DriveSample
{
    double speed = 0.0;
    double rpm = 0.0;
    int gear = 0;
    double throttle = 0.0;
    double accelerator = 0.0;
    double brake = 0.0;
    double steer = 0.0;
    double accelX = 0.0;
    double accelY = 0.0;
    double yawRate = 0.0;
    double engineTorque = 0.0;
    double gearboxTorque = 0.0;
    double coolant = 0.0;
    double oil = 0.0;
    double ambient = 0.0;
    double battery = 0.0;
    double fuelUsed = 0.0;
    double fuel = 0.0;
    double fuelRange = 0.0;
    double fuelLevel = 0.0;
    double fuelLamp = 0.0;
    double wheelFL = 0.0;
    double wheelFR = 0.0;
    double wheelRL = 0.0;
    double wheelRR = 0.0;
    double brakeFront = 0.0;
    double brakeRear = 0.0;
    double brakeStatus = 0.0;
    double highBeam = 0.0;
    double turnLeft = 0.0;
    double turnRight = 0.0;
    double maxRpm = 7000.0;
};

double smoothstep(double u)
{
    u = std::clamp(u, 0.0, 1.0);
    return u * u * (3.0 - 2.0 * u);
}

int gearForSpeed(double speed)
{
    if (speed < 2.0)
        return 0;
    if (speed < 28.0)
        return 1;
    if (speed < 46.0)
        return 2;
    if (speed < 66.0)
        return 3;
    if (speed < 90.0)
        return 4;
    if (speed < 112.0)
        return 5;
    if (speed < 134.0)
        return 6;
    if (speed < 152.0)
        return 7;
    return 8;
}

DriveSample makeSample(double t)
{
    DriveSample s;

    double speed = 0.0;
    double throttle = 0.0;
    double brake = 0.0;
    double steer = 0.0;

    if (t < 4.0) {
        speed = 0.0;
        throttle = 4.0;
    } else if (t < 9.0) {
        const double u = smoothstep((t - 4.0) / 5.0);
        speed = 85.0 * u;
        throttle = 92.0;
    } else if (t < 28.0) {
        const double u = smoothstep((t - 9.0) / 19.0);
        speed = 85.0 + 75.0 * u;
        throttle = 70.0 - 22.0 * u;
    } else if (t < 42.0) {
        const double u = (t - 28.0) / 14.0;
        speed = 160.0 + 3.0 * std::sin(u * 8.0);
        throttle = 24.0 + 5.0 * std::sin(u * 11.0);
    } else if (t < 54.0) {
        const double u = smoothstep((t - 42.0) / 12.0);
        speed = 160.0 - 115.0 * u;
        throttle = 0.0;
        brake = 70.0 * u;
    } else if (t < 68.0) {
        const double u = (t - 54.0) / 14.0;
        speed = 45.0 + 8.0 * std::sin(u * 2.0 * kPi);
        throttle = 18.0 + 3.0 * std::sin(u * 2.0 * kPi);
        brake = std::fmod(t, 3.0) < 0.35 ? 4.0 : 0.0;
        steer = 125.0 * std::sin(u * 2.0 * kPi);
    } else if (t < 76.0) {
        const double u = smoothstep((t - 68.0) / 8.0);
        speed = 48.0 + 62.0 * u;
        throttle = 62.0;
    } else {
        const double u = smoothstep((t - 76.0) / 4.0);
        speed = 110.0 * (1.0 - u);
        throttle = 0.0;
        brake = 55.0;
    }

    s.speed = speed;
    s.throttle = throttle;
    s.accelerator = std::min(100.0, throttle * 1.08);
    s.brake = brake;
    s.steer = steer;

    s.gear = gearForSpeed(speed);
    static constexpr double topSpeed[9] = {0.0, 42.0, 70.0, 100.0, 132.0, 165.0, 200.0, 235.0, 270.0};
    if (s.gear == 0) {
        s.rpm = 850.0 + throttle * 2.0;
    } else {
        s.rpm = 850.0
              + (speed / topSpeed[s.gear]) * (6500.0 - 850.0)
              + throttle * 4.0
              + 18.0 * std::sin(t * 13.0);
    }
    s.rpm = std::clamp(s.rpm, 800.0, 7000.0);

    s.engineTorque = -120.0 + throttle * 5.5;
    s.gearboxTorque = s.engineTorque * 0.92;
    s.brakeFront = brake * 0.85;
    s.brakeRear = brake * 0.42;
    s.brakeStatus = brake > 2.0 ? 1.0 : 0.0;

    s.accelX = std::clamp(0.0072 * (throttle - brake) + 0.015 * std::sin(t * 7.0), -0.9, 0.9);
    s.accelY = std::clamp((steer / 125.0) * 0.55 + 0.01 * std::sin(t * 5.0), -0.9, 0.9);
    s.yawRate = (steer / 125.0) * 24.0 + 0.4 * std::sin(t * 3.0);

    s.coolant = 89.0 + 2.0 * std::sin(t * 0.08);
    s.oil = s.coolant + 7.0 + 1.5 * std::sin(t * 0.05 + 1.2);
    s.ambient = 22.4 + 0.3 * std::sin(t * 0.04);
    s.battery = 14.12 + 0.07 * std::sin(t * 0.8);

    if (speed < 0.5) {
        s.wheelFL = s.wheelFR = s.wheelRL = s.wheelRR = 0.0;
    } else {
        const double wobble = 0.2 * std::sin(t * 3.0);
        s.wheelFL = std::max(0.0, speed + 0.4 + wobble);
        s.wheelFR = std::max(0.0, speed + 0.25 + 0.5 * wobble);
        s.wheelRL = std::max(0.0, speed - 0.2);
        s.wheelRR = std::max(0.0, speed - 0.1);
    }

    s.highBeam = (t >= 28.0 && t < 42.0) ? 1.0 : 0.0;
    const bool blink = std::fmod(t, 0.9) < 0.45;
    s.turnLeft = (blink && t >= 54.0 && t < 61.0) ? 1.0 : 0.0;
    s.turnRight = (blink && t >= 61.0 && t < 68.0) ? 1.0 : 0.0;

    return s;
}

} // namespace

MockTelemetrySource::MockTelemetrySource(QObject *parent)
    : TelemetrySource(parent)
{
    m_timer.setInterval(10);
    connect(&m_timer, &QTimer::timeout, this, &MockTelemetrySource::tick);
}

void MockTelemetrySource::start()
{
    if (state() == State::Running)
        return;

    m_tick = 0;
    m_fuelUsed = 0.0;
    setVehicleName(QStringLiteral("Demo vehicle"));
    setState(State::Starting, QStringLiteral("Starting demo"));
    m_clock.start();
    m_timer.start();
    setState(State::Running, QStringLiteral("Synthetic drive cycle"));
}

void MockTelemetrySource::stop()
{
    m_timer.stop();
    setState(State::Stopped, QStringLiteral("Stopped"));
}

QString MockTelemetrySource::name() const
{
    return QStringLiteral("mock");
}

QString MockTelemetrySource::description() const
{
    return QStringLiteral("Synthetic drive cycle");
}

QString MockTelemetrySource::label() const
{
    return QStringLiteral("DEMO");
}

void MockTelemetrySource::emitValue(sig::Tag tag, double value)
{
    TelemetryValue v;
    v.tag = tag;
    v.value = value;
    emit valueReceived(v);
}

void MockTelemetrySource::tick()
{
    const double elapsed = m_clock.elapsed() / 1000.0;
    const double t = std::fmod(elapsed, kCycleSeconds);
    DriveSample s = makeSample(t);

    m_fuelUsed += 0.00003 * (0.35 + s.throttle / 100.0);
    s.fuelUsed = m_fuelUsed;
    s.fuel = std::max(0.0, 28.0 - m_fuelUsed * 0.25);
    s.fuelRange = std::clamp(s.fuel * 8.0, 0.0, 255.0);
    s.fuelLevel = std::clamp(s.fuel / 28.0 * 100.0, 0.0, 100.0);
    s.fuelLamp = s.fuel < 8.0 ? 1.0 : 0.0;

    ++m_tick;

    if (m_tick % 5 == 0) {
        emitValue(sig::Rpm, s.rpm);
        emitValue(sig::Speed, s.speed);
        emitValue(sig::Gear, s.gear);
        emitValue(sig::Throttle, s.throttle);
        emitValue(sig::Accelerator, s.accelerator);
        emitValue(sig::Brake, s.brake);
        emitValue(sig::Steering, s.steer);
    }

    if (m_tick % 10 == 0) {
        emitValue(sig::WheelFrontLeft, s.wheelFL);
        emitValue(sig::WheelFrontRight, s.wheelFR);
        emitValue(sig::WheelRearLeft, s.wheelRL);
        emitValue(sig::WheelRearRight, s.wheelRR);
        emitValue(sig::EngineTorque, s.engineTorque);
        emitValue(sig::GearboxTorque, s.gearboxTorque);
        emitValue(sig::AccelX, s.accelX);
        emitValue(sig::AccelY, s.accelY);
        emitValue(sig::YawRate, s.yawRate);
        emitValue(sig::BrakePressureFront, s.brakeFront);
        emitValue(sig::BrakePressureRear, s.brakeRear);
        emitValue(sig::MaxRpm, s.maxRpm);
    }

    if (m_tick % 50 == 0) {
        emitValue(sig::CoolantTemp, s.coolant);
        emitValue(sig::OilTemp, s.oil);
        emitValue(sig::AmbientTemp, s.ambient);
        emitValue(sig::BatteryVoltage, s.battery);
        emitValue(sig::FuelRange, s.fuelRange);
        emitValue(sig::FuelUsed, s.fuelUsed);
        emitValue(sig::FuelLevel, s.fuelLevel);
        emitValue(sig::FuelLamp, s.fuelLamp);
        emitValue(sig::HighBeam, s.highBeam);
        emitValue(sig::TurnLeft, s.turnLeft);
        emitValue(sig::TurnRight, s.turnRight);
        emitValue(sig::Abs, 0.0);
        emitValue(sig::Asc, 0.0);
        emitValue(sig::BrakeStatus, s.brakeStatus);
        emitValue(sig::EngineLoad, s.throttle);
        emitValue(sig::IntakeTemp, s.ambient + 12.0);
        emitValue(sig::Boost, std::max(0.0, s.throttle / 100.0 * 1.2 - 0.15));
        emitValue(sig::OilPressure, 1.2 + s.rpm / 3000.0);
        emitValue(sig::FuelPressure, 3.5);
        emitValue(sig::Lambda, 1.0 - s.throttle / 100.0 * 0.12);
        emitValue(sig::Egt, 450.0 + s.throttle * 3.5);
        emitValue(sig::Odometer, 12345.0 + t * 0.02);
    }
}
