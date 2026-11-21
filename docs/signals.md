# G29/J29 PT-CAN signal map

Source: [The SECRET Ingredient! — BMW PT-CAN Messages G29/J29](https://thesecretingredient.neocities.org/bmw/can/g29/).
The implementation is `src/decode/bmwptcan.cpp`; this document explains the rules
and the rows that still need validation against a real log.

## Decode rules

The reference table mixes offset units. `SignalDef::bitMode` selects the rule:

| `Bit Mode` | Offset / length | Extraction |
|---|---|---|
| `Yes` | bits, MSB-first (bit 0 = MSB of byte 0) | the field may straddle bytes/nibbles |
| empty | bytes | big-endian, `length` bytes |

All raw values are treated as **unsigned** unless `signedRaw` is set on a row.
This is required for the published bias constants: engine torque
`raw / 2 - 1023.5` only produces a negative torque region when `raw` is unsigned.
The `add` value is the signed bias, not a sign extension.

The value formula is:

```
value = raw * multiply / divide + add
```

The demo source and the round-trip tests use the inverse formula. Invalid/short
frames are rejected rather than partially decoded.

## Map

| CAN ID | Signal | Unit | Offset | Length | Bit mode | Multiply | Divide | Add |
|---|---|---|---|---|---|---|---|---|
| 0x0A5 | RPM | 1/min | 5 | 2 | | 1 | 4 | 0 |
| 0x0A5 | Engine torque | Nm | 16 | 12 | Yes | 1 | 2 | -1023.5 |
| 0x0A5 | Gearbox torque | Nm | 28 | 12 | Yes | 1 | 2 | -1023.5 |
| 0x0D9 | Accelerator | % | 16 | 12 | Yes | 1 | 40 | 0 |
| 0x0D9 | Throttle | % | 32 | 12 | Yes | 1 | 40 | 0 |
| 0x0EF | Brake pressure front | bar | 2 | 2 | | -3 | 200 | 480 |
| 0x0EF | Brake | % | 3 | 1 | | -2 | 1 | 250 |
| 0x0EF | Brake pressure rear | bar | 5 | 2 | | -3 | 200 | 480 |
| 0x173 | ABS | # | 28 | 12 | Yes | 1 | 1 | 0 |
| 0x173 | ASC | # | 40 | 12 | Yes | 1 | 1 | 0 |
| 0x173 | Brake status | # | 56 | 6 | Yes | 1 | 1 | 0 |
| 0x199 | Acceleration X | g | 2 | 2 | | -1 | 5000 | 6.6259 |
| 0x19A | Acceleration Y | g | 2 | 2 | | -1 | 5000 | 6.6259 |
| 0x19F | Yaw rate | °/s | 2 | 2 | | -1 | 200 | 163.84 |
| 0x1A1 | Speed | km/h | 2 | 2 | | 1 | 64 | 0 |
| 0x1EE | High beam | | 5 | 1 | Yes | 1 | 1 | 0 |
| 0x1F6 | Indicator left | | 4 | 1 | Yes | 1 | 1 | 0 |
| 0x1F6 | Indicator right | | 5 | 1 | Yes | 1 | 1 | 0 |
| 0x254 | Wheel speed RL | km/h | 0 | 2 | | 1 | 64 | -511.98001 |
| 0x254 | Wheel speed RR | km/h | 2 | 2 | | 1 | 64 | -511.98001 |
| 0x254 | Wheel speed FL | km/h | 4 | 2 | | 1 | 64 | -511.98001 |
| 0x254 | Wheel speed FR | km/h | 6 | 2 | | 1 | 64 | -511.98001 |
| 0x281 | Battery voltage | mV | 1 | 12 | Yes | 15 | 1 | 0 |
| 0x2C4 | Fuel raw | # | 0 | 2 | | 1 | 1 | 0 |
| 0x2CA | Ambient temperature | °C | 0 | 1 | | 1 | 2 | -40 |
| 0x301 | Steering | ° | 2 | 2 | | -1 | 22.75 | 1440 |
| 0x330 | Fuel used | L | 4 | 1 | | 1 | 1 | 0 |
| 0x330 | Fuel indicator lamp | | 40 | 1 | Yes | 1 | 1 | 0 |
| 0x330 | Fuel range | km | 48 | 12 | Yes | 1 | 16 | 0 |
| 0x3F9 | Coolant temperature | °C | 4 | 1 | | 1 | 1 | -48 |
| 0x3F9 | Oil temperature | °C | 5 | 1 | | 1 | 1 | -48 |
| 0x3F9 | Maximum RPM | 1/min | 7 | 1 | | 50 | 1 | 0 |
| 0x3F9 | Gear | | 48 | 4 | Yes | 1 | 1 | -4 |

Implementation notes:

- Battery is exposed in volts: `raw * 15 / 1000`, not millivolts.
- `Maximum RPM` uses `raw * 50`; see below.
- The raw value in the debug overlay is the field value before scaling.

## Rows that need a real car/log

### 1. Maximum RPM (0x3F9, byte 7)

The reference page says `multiply 1 / divide 1 / add 50`. Applied to the page's
own sample frame (`0xA3`), that yields **213 rpm**, which cannot be a maximum RPM.
The only plausible reading is `raw * 50`, which makes the sample **8150 rpm**.
This implementation uses `* 50` and documents the deviation. `VehicleState` also
drops values outside 5000–9000 rpm; QML then falls back to 7000 rpm for the
shift-light reference. Verify with a live frame before trusting the redline.

### 2. Brake pressure front vs. brake % (0x0EF)

The published entries overlap:

- front pressure: offset 2, length 2 → bytes 2–3
- brake: offset 3, length 1 → byte 3
- rear pressure: offset 5, length 2 → bytes 5–6

One of these is likely mistranscribed. The app uses **brake %** for the pedal bar
and encodes the brake value last in the mock frame, so the pedal display is
correct. The debug overlay shows the overlapping raw values; use a log to decide
whether the pressure field is really 16 bits or whether the offset is different.

### 3. Gear (0x3F9, bits 48–51)

The published formula `raw - 4` with unsigned raw gives:

| raw | value | UI |
|---|---|---|
| 3 | -1 | R |
| 4 | 0 | N |
| 5…12 | 1…8 | digit |
| 15 | 11 | clamped to 8 |

The sample frame has nibble `3` and decodes to -1. If a log shows reverse or
drive differently, set `signedRaw` on the gear row and re-check. The UI label
mapping lives in `VehicleState::flush()`.

### 4. Acceleration / yaw bias constants

`6.6259` for the two accelerometers and `163.84` for yaw rate look like sensor
zero calibration (yaw `163.84 = 32768 / 200`). Keep them as published until a
known-flat, stationary log can confirm the zero points.

### 5. Fuel range width

12 bits at `*1/16` gives a maximum of **255.9 km**. If the car can show a longer
range, the field is probably wider than 12 bits or has a different scale. The
dashboard's fuel bar is scaled to 256 km for now.

### 6. Fuel raw

`0x2C4` is a raw tank-level count with no published calibration curve. The app
does not show a fuel-level percentage; it shows range, fuel used, and the fuel
lamp. The raw value is available in the debug overlay.

## How to validate

1. Run the app with `--debug` and look at the raw payload + decoded values.
2. Record a PT-CAN log in the car (the firmware project can add a sniffing mode).
3. Compare the raw bytes and the decoded values with the reference table.
4. Fix a row in `src/decode/bmwptcan.cpp` and add/adjust a case in
   `tests/tst_decoder.cpp`.

The published sample used by the tests:

```
0x3F9: 4E FE 82 3E 81 88 33 A3
       coolant = 81 °C, oil = 88 °C, max RPM = 8150 (see note), gear nibble = 3
```
