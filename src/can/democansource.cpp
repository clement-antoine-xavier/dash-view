#include "can/democansource.h"

#include <algorithm>
#include <cmath>

#include "decode/decoder.h"

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
    double brakeFront = 0.0;
    double brakeRear = 0.0;
    double brakeStatus = 0.0;
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
    double fuelRaw = 0.0;
    double fuelLamp = 0.0;
    double wheelFL = 0.0;
    double wheelFR = 0.0;
    double wheelRL = 0.0;
    double wheelRR = 0.0;
    double highBeam = 0.0;
    double turnLeft = 0.0;
    double turnRight = 0.0;
    double abs = 0.0;
    double asc = 0.0;
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
        // Idle.
        speed = 0.0;
        throttle = 4.0;
    } else if (t < 9.0) {
        // Launch.
        const double u = smoothstep((t - 4.0) / 5.0);
        speed = 85.0 * u;
        throttle = 92.0;
    } else if (t < 28.0) {
        // Pull to 160 km/h.
        const double u = smoothstep((t - 9.0) / 19.0);
        speed = 85.0 + 75.0 * u;
        throttle = 70.0 - 22.0 * u;
    } else if (t < 42.0) {
        // Cruise.
        const double u = (t - 28.0) / 14.0;
        speed = 160.0 + 3.0 * std::sin(u * 8.0);
        throttle = 24.0 + 5.0 * std::sin(u * 11.0);
    } else if (t < 54.0) {
        // Brake into a corner.
        const double u = smoothstep((t - 42.0) / 12.0);
        speed = 160.0 - 115.0 * u;
        throttle = 0.0;
        brake = 70.0 * u;
    } else if (t < 68.0) {
        // Corner.
        const double u = (t - 54.0) / 14.0;
        speed = 45.0 + 8.0 * std::sin(u * 2.0 * kPi);
        throttle = 18.0 + 3.0 * std::sin(u * 2.0 * kPi);
        brake = std::fmod(t, 3.0) < 0.35 ? 4.0 : 0.0;
        steer = 125.0 * std::sin(u * 2.0 * kPi);
    } else if (t < 76.0) {
        // Corner exit.
        const double u = smoothstep((t - 68.0) / 8.0);
        speed = 48.0 + 62.0 * u;
        throttle = 62.0;
    } else {
        // Stop for the end of the loop.
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

void writeBytes(std::array<quint8, 8> &data, int offset, int length, quint64 raw)
{
    for (int i = 0; i < length; ++i)
        data[offset + i] = static_cast<quint8>((raw >> (8 * (length - 1 - i))) & 0xFFu);
}

void writeBits(std::array<quint8, 8> &data, int startBit, int length, quint64 raw)
{
    for (int i = 0; i < length; ++i) {
        const int bit = startBit + i;
        const quint8 mask = static_cast<quint8>(1u << (7 - (bit % 8)));
        if ((raw >> (length - 1 - i)) & 1u)
            data[bit / 8] |= mask;
        else
            data[bit / 8] &= static_cast<quint8>(~mask);
    }
}

} // namespace

DemoCanSource::DemoCanSource(QObject *parent)
    : CanSource(parent)
{
    m_timer.setInterval(10);
    connect(&m_timer, &QTimer::timeout, this, &DemoCanSource::tick);
}

void DemoCanSource::start()
{
    if (state() == State::Running)
        return;

    m_tick = 0;
    m_fuelUsed = 0.0;
    m_clock.start();
    m_timer.start();
    setState(State::Running, QStringLiteral("Synthetic G29 PT-CAN drive cycle"));
}

void DemoCanSource::stop()
{
    m_timer.stop();
    setState(State::Stopped, QStringLiteral("Stopped"));
}

QString DemoCanSource::name() const
{
    return QStringLiteral("demo");
}

QString DemoCanSource::description() const
{
    return QStringLiteral("Synthetic G29 drive cycle");
}

void DemoCanSource::tick()
{
    const double elapsed = m_clock.elapsed() / 1000.0;
    const double t = std::fmod(elapsed, kCycleSeconds);
    DriveSample s = makeSample(t);

    m_fuelUsed += 0.00003 * (0.35 + s.throttle / 100.0);
    s.fuelUsed = m_fuelUsed;
    s.fuel = std::max(0.0, 28.0 - m_fuelUsed * 0.25);
    s.fuelRange = std::clamp(s.fuel * 8.0, 0.0, 255.0);
    s.fuelRaw = std::clamp(20000.0 + s.fuel * 700.0, 0.0, 65535.0);
    s.fuelLamp = s.fuel < 8.0 ? 1.0 : 0.0;

    ++m_tick;
    const qint64 ts = m_clock.elapsed();

    if (m_tick % 1 == 0) {
        std::array<quint8, 8> d{};
        writeSignal(SignalId::Rpm, s.rpm, d);
        writeSignal(SignalId::EngineTorque, s.engineTorque, d);
        writeSignal(SignalId::GearboxTorque, s.gearboxTorque, d);
        emitFrame(0x0A5, d, ts);
    }

    if (m_tick % 2 == 0) {
        {
            std::array<quint8, 8> d{};
            writeSignal(SignalId::Accelerator, s.accelerator, d);
            writeSignal(SignalId::Throttle, s.throttle, d);
            emitFrame(0x0D9, d, ts);
        }
        {
            // NOTE: the published 0x0EF entries overlap (front pressure spans
            // bytes 2-3, brake % is byte 3). Encode the brake value last so the
            // pedal bar is correct; the front pressure readout is debug-only.
            std::array<quint8, 8> d{};
            writeSignal(SignalId::BrakePressureFront, s.brakeFront, d);
            writeSignal(SignalId::BrakePercent, s.brake, d);
            writeSignal(SignalId::BrakePressureRear, s.brakeRear, d);
            emitFrame(0x0EF, d, ts);
        }
        {
            std::array<quint8, 8> d{};
            writeSignal(SignalId::AccelX, s.accelX, d);
            emitFrame(0x199, d, ts);
        }
        {
            std::array<quint8, 8> d{};
            writeSignal(SignalId::AccelY, s.accelY, d);
            emitFrame(0x19A, d, ts);
        }
        {
            std::array<quint8, 8> d{};
            writeSignal(SignalId::YawRate, s.yawRate, d);
            emitFrame(0x19F, d, ts);
        }
        {
            std::array<quint8, 8> d{};
            writeSignal(SignalId::Speed, s.speed, d);
            emitFrame(0x1A1, d, ts);
        }
        {
            std::array<quint8, 8> d{};
            writeSignal(SignalId::WheelRearLeft, s.wheelRL, d);
            writeSignal(SignalId::WheelRearRight, s.wheelRR, d);
            writeSignal(SignalId::WheelFrontLeft, s.wheelFL, d);
            writeSignal(SignalId::WheelFrontRight, s.wheelFR, d);
            emitFrame(0x254, d, ts);
        }
        {
            std::array<quint8, 8> d{};
            writeSignal(SignalId::Steering, s.steer, d);
            emitFrame(0x301, d, ts);
        }
    }

    if (m_tick % 5 == 0) {
        {
            std::array<quint8, 8> d{};
            writeSignal(SignalId::Abs, s.abs, d);
            writeSignal(SignalId::Asc, s.asc, d);
            writeSignal(SignalId::BrakeStatus, s.brakeStatus, d);
            emitFrame(0x173, d, ts);
        }
        {
            std::array<quint8, 8> d{};
            writeSignal(SignalId::CoolantTemp, s.coolant, d);
            writeSignal(SignalId::OilTemp, s.oil, d);
            writeSignal(SignalId::MaxRpm, s.maxRpm, d);
            writeSignal(SignalId::Gear, static_cast<double>(s.gear), d);
            emitFrame(0x3F9, d, ts);
        }
    }

    if (m_tick % 10 == 0) {
        {
            std::array<quint8, 8> d{};
            writeSignal(SignalId::HighBeam, s.highBeam, d);
            emitFrame(0x1EE, d, ts);
        }
        {
            std::array<quint8, 8> d{};
            writeSignal(SignalId::TurnLeft, s.turnLeft, d);
            writeSignal(SignalId::TurnRight, s.turnRight, d);
            emitFrame(0x1F6, d, ts);
        }
    }

    if (m_tick % 50 == 0) {
        {
            std::array<quint8, 8> d{};
            writeSignal(SignalId::BatteryVoltage, s.battery, d);
            emitFrame(0x281, d, ts);
        }
        {
            std::array<quint8, 8> d{};
            writeSignal(SignalId::FuelRaw, s.fuelRaw, d);
            emitFrame(0x2C4, d, ts);
        }
        {
            std::array<quint8, 8> d{};
            writeSignal(SignalId::FuelUsed, s.fuelUsed, d);
            writeSignal(SignalId::FuelLamp, s.fuelLamp, d);
            writeSignal(SignalId::FuelRange, s.fuelRange, d);
            emitFrame(0x330, d, ts);
        }
    }

    if (m_tick % 100 == 0) {
        std::array<quint8, 8> d{};
        writeSignal(SignalId::AmbientTemp, s.ambient, d);
        emitFrame(0x2CA, d, ts);
    }
}

void DemoCanSource::emitFrame(quint16 id, const std::array<quint8, 8> &data, qint64 timestampMs)
{
    CanFrame frame;
    frame.id = id;
    frame.dlc = 8;
    frame.data = data;
    frame.timestampMs = timestampMs;
    emit frameReceived(frame);
}

void DemoCanSource::writeSignal(SignalId id, double value, std::array<quint8, 8> &data) const
{
    const SignalDef *def = canmap::definition(id);
    if (!def)
        return;

    const quint64 raw = decoder::encodeRaw(*def, value);
    if (def->bitMode)
        writeBits(data, def->offset, def->length, raw);
    else
        writeBytes(data, def->offset, def->length, raw);
}
