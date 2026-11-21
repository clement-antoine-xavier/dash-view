# Example: BMW G29/J29 PT-CAN

This is a **reference for the Arduino firmware project**, not code used by Dash
View. The app only consumes tagged values; the firmware decodes these raw CAN
messages and emits the corresponding tags. If you are adding another vehicle, use
this as an example of what the firmware side needs to do.

Source: [The SECRET Ingredient! — BMW PT-CAN Messages G29/J29](https://thesecretingredient.neocities.org/bmw/can/g29/).

## Hardware

### PT-CAN, not OBD-II

The table is for the **PT-CAN** (powertrain CAN, 500 kbit/s). The OBD-II socket on
the G29 is on the **D-CAN** and carries diagnostic traffic, not these broadcasts.
The OBD socket will not produce this data.

### Tap location

The reference taps the PT-CAN in the **passenger outer footwell**, behind the
kick panel:

- **CAN HI** — yellow/white
- **CAN LO** — yellow/black

Use a proper tap, keep the stub short, and secure the harness.

### Termination

A CAN bus is terminated at both physical ends, normally with 120 Ω each. A tap is
a stub, not a new end. Do not add a third 120 Ω termination. Check the MKR CAN
shield schematic: if it has an onboard 120 Ω resistor, remove/disable it for a
mid-bus tap. With the bus off and the tap disconnected, a terminated bus reads
about 60 Ω across CAN H/L.

### Power and listen-only

- Power the MKR from USB during development or from the shield's fused 12 V buck
  in the car; keep a common ground.
- Do not backfeed the car or power from two sources at once.
- Configure the MCP2515 for **500 kbit/s, 16 MHz, listen-only**. Never transmit
  application frames on the PT-CAN. Listen-only also avoids driving the CAN ACK
  bit.
- The Arduino MKR CAN Shield uses the MCP2515 controller and TJA1049 transceiver
  with **CS = D3** and **INT = D7**.

The BLE side (device name, service and 6-byte tagged records) is specified in
[`../telemetry-protocol.md`](../telemetry-protocol.md).

## Raw map and tag mapping

Offset units switch with `Bit Mode`:

- `Yes`: offset/length are **bits**, MSB-first (bit 0 = MSB of byte 0).
- empty: offset/length are **bytes**, big-endian.

Raw values are unsigned unless noted; `add` is the signed bias. The formula is
`value = raw * multiply / divide + add`. The firmware should emit the result in
the app's physical units.

| CAN ID | Raw signal | Unit | Offset | Length | Bit mode | Multiply | Divide | Add | App tag |
|---|---|---|---|---|---|---|---|---|---|
| 0x0A5 | RPM | 1/min | 5 | 2 | | 1 | 4 | 0 | 0x01 `rpm` |
| 0x0A5 | Engine torque | Nm | 16 | 12 | Yes | 1 | 2 | -1023.5 | 0x08 `engineTorque` |
| 0x0A5 | Gearbox torque | Nm | 28 | 12 | Yes | 1 | 2 | -1023.5 | 0x09 `gearboxTorque` |
| 0x0D9 | Accelerator | % | 16 | 12 | Yes | 1 | 40 | 0 | 0x05 `accelerator` |
| 0x0D9 | Throttle | % | 32 | 12 | Yes | 1 | 40 | 0 | 0x04 `throttle` |
| 0x0EF | Brake pressure front | bar | 2 | 2 | | -3 | 200 | 480 | 0x13 `brakePressureFront` |
| 0x0EF | Brake | % | 3 | 1 | | -2 | 1 | 250 | 0x06 `brakePct` |
| 0x0EF | Brake pressure rear | bar | 5 | 2 | | -3 | 200 | 480 | 0x14 `brakePressureRear` |
| 0x173 | ABS | # | 28 | 12 | Yes | 1 | 1 | 0 | 0x1F `abs` |
| 0x173 | ASC | # | 40 | 12 | Yes | 1 | 1 | 0 | 0x20 `asc` |
| 0x173 | Brake status | # | 56 | 6 | Yes | 1 | 1 | 0 | 0x21 `brakeStatus` |
| 0x199 | Acceleration X | g | 2 | 2 | | -1 | 5000 | 6.6259 | 0x16 `accelX` |
| 0x19A | Acceleration Y | g | 2 | 2 | | -1 | 5000 | 6.6259 | 0x17 `accelY` |
| 0x19F | Yaw rate | °/s | 2 | 2 | | -1 | 200 | 163.84 | 0x15 `yawRate` |
| 0x1A1 | Speed | km/h | 2 | 2 | | 1 | 64 | 0 | 0x02 `speed` |
| 0x1EE | High beam | | 5 | 1 | Yes | 1 | 1 | 0 | 0x1C `highBeam` |
| 0x1F6 | Indicator left | | 4 | 1 | Yes | 1 | 1 | 0 | 0x1D `turnLeft` |
| 0x1F6 | Indicator right | | 5 | 1 | Yes | 1 | 1 | 0 | 0x1E `turnRight` |
| 0x254 | Wheel speed RL | km/h | 0 | 2 | | 1 | 64 | -511.98001 | 0x1A `wheelRearLeft` |
| 0x254 | Wheel speed RR | km/h | 2 | 2 | | 1 | 64 | -511.98001 | 0x1B `wheelRearRight` |
| 0x254 | Wheel speed FL | km/h | 4 | 2 | | 1 | 64 | -511.98001 | 0x18 `wheelFrontLeft` |
| 0x254 | Wheel speed FR | km/h | 6 | 2 | | 1 | 64 | -511.98001 | 0x19 `wheelFrontRight` |
| 0x281 | Battery voltage | mV | 1 | 12 | Yes | 15 | 1 | 0 | 0x0D `batteryVoltage` (divide by 1000 for V) |
| 0x2C4 | Fuel raw | # | 0 | 2 | | 1 | 1 | 0 | custom 0x80, or `fuelLevel` with a calibration |
| 0x2CA | Ambient temperature | °C | 0 | 1 | | 1 | 2 | -40 | 0x0C `ambientTemp` |
| 0x301 | Steering | ° | 2 | 2 | | -1 | 22.75 | 1440 | 0x07 `steering` |
| 0x330 | Fuel used | L | 4 | 1 | | 1 | 1 | 0 | 0x0F `fuelUsed` |
| 0x330 | Fuel indicator lamp | | 40 | 1 | Yes | 1 | 1 | 0 | 0x11 `fuelLamp` |
| 0x330 | Fuel range | km | 48 | 12 | Yes | 1 | 16 | 0 | 0x0E `fuelRange` |
| 0x3F9 | Coolant temperature | °C | 4 | 1 | | 1 | 1 | -48 | 0x0A `coolantTemp` |
| 0x3F9 | Oil temperature | °C | 5 | 1 | | 1 | 1 | -48 | 0x0B `oilTemp` |
| 0x3F9 | Maximum RPM | 1/min | 7 | 1 | | 50 | 1 | 0 | 0x12 `maxRpm` |
| 0x3F9 | Gear | | 48 | 4 | Yes | 1 | 1 | -4 | 0x03 `gear` |

### Rows that need validation

The reference table has a few quirks; verify them with a real frame before
trusting the display:

1. **Maximum RPM**: the page says `multiply 1 / divide 1 / add 50`; applied to
   the page's own sample byte `0xA3` that gives 213 rpm. The only plausible
   reading is `raw * 50` → **8150 rpm**. The table above uses `* 50`. The app
   accepts any positive `maxRpm`, so the firmware should send a value verified
   against a log (or 7000 as a safe fallback).
2. **0x0EF overlap**: front pressure (bytes 2–3) and brake % (byte 3) overlap.
   One entry is likely mistranscribed. Prefer brake % for the pedal bar; use the
   debug overlay to inspect the raw bytes.
3. **Gear signedness**: `raw - 4` with unsigned raw gives the sample nibble 3 →
   -1. Normalize to -1 = R, 0 = N, 1..N = forward in the firmware.
4. **Acceleration/yaw bias**: `6.6259` and `163.84` look like sensor zero
   calibration (163.84 = 32768 / 200). Keep as published until a stationary log
   confirms the zero point.
5. **Fuel range width**: 12 bits at `*1/16` maxes at 255.9 km. If the car can show
   more, the field is wider or scaled differently. The app's fuel bar assumes a
   600 km full scale; this is only a visual.
6. **Fuel raw**: no calibration curve; emit it as custom tag `0x80` if you want
   to collect it, or map it to `fuelLevel` once you have a curve.

The published sample frame used in the firmware notes:

```
0x3F9: 4E FE 82 3E 81 88 33 A3
       coolant = 81 °C, oil = 88 °C, max RPM = 8150 (with *50), gear nibble = 3
```

## What the firmware should emit

For this vehicle, send the mapped tags at the rates suggested in
[`../telemetry-protocol.md`](../telemetry-protocol.md). In particular:

- Send `maxRpm` early so the tach scale is right.
- Send `gear` as a normalized number, not the raw nibble.
- Convert battery mV to volts.
- Map the accelerator and throttle separately; the dashboard shows both.
- Keep the CAN side listen-only; the app never talks to the vehicle bus.
