# Tagged telemetry protocol

Dash View is vehicle-agnostic: it never decodes raw CAN. The Arduino firmware
(the separate project) owns the vehicle-specific CAN map, converts signals to
physical values, and sends them as a **tagged union**.

```
vehicle CAN ──> Arduino (vehicle profile) ──> tagged BLE values ──> Dash View
```

This document is the contract between the firmware and the app. The app already
implements the central/decoder side; the firmware side is not part of this
repository.

## GATT

| Item | UUID |
|---|---|
| Service | `6b8d0001-1f2e-4a3b-9c4d-5e6f7a8b9c0d` |
| Telemetry (notify) | `6b8d0002-1f2e-4a3b-9c4d-5e6f7a8b9c0d` |
| Vehicle info (read) | `6b8d0003-1f2e-4a3b-9c4d-5e6f7a8b9c0d` |

- Advertised device name: `DashView` (the app also matches the service UUID, and
  `--ble-name <substring>` overrides the name filter).
- The telemetry characteristic is notify-only, maximum value length 20 bytes.
- No pairing or authentication is required for v1. The data is broadcast vehicle
  telemetry.

## Record layout

A telemetry notification is a concatenation of **1 to 3 complete records**
(6/12/18 bytes). It is never padded: the receiver splits the notification into
6-byte records. `ArduinoBLE`'s usable notification payload is 20 bytes, so 3
records is the maximum.

Each record is 6 bytes:

| Byte | Meaning |
|---|---|
| 0 | tag (see catalog) |
| 1 | type: `0` float32, `1` int32, `2` uint32, `3` bool |
| 2..5 | value, little-endian |

The value is an IEEE-754 float32 for type 0, a two's-complement int32 for type 1,
an unsigned 32-bit integer for type 2, and a bool in byte 2 for type 3.

**Do not send a C struct directly.** Struct padding/alignment can change
`sizeof(TaggedValue)`. Serialize exactly six bytes.

Example — RPM 858.0 as float32:

```
01 00 00 80 56 44
^  ^  ^^^^^^^^^^^
|  |  858.0f, little-endian
|  type 0 (float32)
tag 0x01 (RPM)
```

Example — gear 4 as int32:

```
03 01 04 00 00 00
```

A notification can contain, for example, `RPM`, `Speed`, and `Gear` records in
one 18-byte notification.

## Tag catalog

Tags are the app-facing roles. The dashboard displays the standard tags it knows;
unknown/custom tags are recorded and shown in the debug overlay (press **D**).

| Tag | Key | Label | Unit | Notes |
|---|---|---|---|---|
| 0x01 | `rpm` | RPM | rpm | |
| 0x02 | `speed` | Speed | km/h | |
| 0x03 | `gear` | Gear | | use -1 = R, 0 = N, 1..N = forward gears |
| 0x04 | `throttle` | Throttle | % | |
| 0x05 | `accelerator` | Accelerator | % | |
| 0x06 | `brakePct` | Brake | % | |
| 0x07 | `steering` | Steering | ° | |
| 0x08 | `engineTorque` | Engine torque | Nm | |
| 0x09 | `gearboxTorque` | Gearbox torque | Nm | |
| 0x0A | `coolantTemp` | Coolant | °C | |
| 0x0B | `oilTemp` | Oil | °C | |
| 0x0C | `ambientTemp` | Ambient | °C | |
| 0x0D | `batteryVoltage` | Battery | V | |
| 0x0E | `fuelRange` | Fuel range | km | |
| 0x0F | `fuelUsed` | Fuel used | L | |
| 0x10 | `fuelLevel` | Fuel level | % | |
| 0x11 | `fuelLamp` | Fuel lamp | | bool |
| 0x12 | `maxRpm` | Max RPM | rpm | send early; scales the tach |
| 0x13 | `brakePressureFront` | Brake pressure F | bar | |
| 0x14 | `brakePressureRear` | Brake pressure R | bar | |
| 0x15 | `yawRate` | Yaw rate | °/s | |
| 0x16 | `accelX` | Accel X | g | longitudinal |
| 0x17 | `accelY` | Accel Y | g | lateral |
| 0x18 | `wheelFrontLeft` | Wheel FL | km/h | |
| 0x19 | `wheelFrontRight` | Wheel FR | km/h | |
| 0x1A | `wheelRearLeft` | Wheel RL | km/h | |
| 0x1B | `wheelRearRight` | Wheel RR | km/h | |
| 0x1C | `highBeam` | High beam | | bool |
| 0x1D | `turnLeft` | Turn left | | bool |
| 0x1E | `turnRight` | Turn right | | bool |
| 0x1F | `abs` | ABS | | bool |
| 0x20 | `asc` | ASC | | bool |
| 0x21 | `brakeStatus` | Brake status | # | |
| 0x22 | `engineLoad` | Engine load | % | |
| 0x23 | `intakeTemp` | Intake temp | °C | |
| 0x24 | `boost` | Boost | bar | |
| 0x25 | `oilPressure` | Oil pressure | bar | |
| 0x26 | `fuelPressure` | Fuel pressure | bar | |
| 0x27 | `lambda` | Lambda | λ | |
| 0x28 | `egt` | EGT | °C | |
| 0x29 | `odometer` | Odometer | km | |
| 0x2A | `batterySoc` | Battery SOC | % | EV/hybrid |
| 0x2B | `powerKw` | Power | kW | EV/hybrid |
| 0x2C | `motorTemp` | Motor temp | °C | EV/hybrid |

Reserved ranges:

- `0x2D..0x7F` — future standard tags.
- `0x80..0xEF` — **vehicle-specific custom tags**. The app records them and
  shows them in the debug overlay with a `Custom 0xNN` label. They are not shown
  on the main dashboard (adding a dashboard widget is an app change).
- `0xF0..0xFF` — reserved for protocol/meta use.

`--list-tags` prints the catalog from the app.

## Vehicle info characteristic

The app reads the optional info characteristic after connecting. It may contain:

```json
{"vehicle":"2019 BMW Z4","protocol":1}
```

The `vehicle` string is shown in the header and the link panel. If the
characteristic is absent or unreadable, the BLE device name is used.

## Rate budget

At a 15–30 ms BLE connection interval with 3 records per notification, the link
can carry roughly 100–200 values/s. Suggested firmware rates:

| Group | Rate |
|---|---|
| RPM, speed, throttle, brake, steering, gear | 10–20 Hz |
| Torques, wheel speeds, acceleration, yaw | 5–10 Hz |
| Temperatures, battery, fuel, lights, maxRpm | 1–5 Hz |

Send `maxRpm` and `gear` early so the dashboard has its scale and gear display.
Rate-limit per tag on the firmware; do not queue without bound. If the app reports
dropped values, lower the rates before adding compression.

## Firmware checklist

1. Decode the vehicle's CAN map into physical values.
2. Normalize gear to -1/0/1..N and booleans to 0/1.
3. Encode each value as a 6-byte record (tag, type, little-endian value).
4. Batch 1–3 records per notification; never pad inside a notification.
5. Advertise the service and name; set the optional info characteristic.
6. Keep the CAN side independent of BLE connection state.
7. Never transmit application frames on the vehicle bus (listen-only).

See [`vehicles/bmw-g29.md`](vehicles/bmw-g29.md) for the G29/J29 example map and
wiring notes.

## App behavior

- `--source ble` starts the central; it scans for `DashView` (or `--ble-name`) or
  the service UUID, connects, enables notifications, and reads the info
  characteristic.
- Malformed records (tag 0, type > 3, truncated) are dropped.
- Values are coalesced to ~30 Hz for the UI and disappear after their per-tag
  timeout, so a lost link shows `—` and a red status pill.
- The **D** overlay shows every catalog tag, raw stream values, age and count.
- The BLE implementation is experimental; the firmware side is not in this
  repository.
