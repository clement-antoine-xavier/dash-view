# Dash View

A vehicle-agnostic Qt 6 / QML dashboard for tagged telemetry from an Arduino.

The car-specific work lives in the **Arduino firmware**: it decodes the vehicle's
CAN bus and sends physical values as a tagged union over BLE. Dash View displays,
records and replays those values. There is no raw CAN decoding in this app, so
adding a vehicle does not require touching the dashboard.

```
vehicle CAN ──> Arduino (per-vehicle profile) ──> tagged BLE ──> Dash View
                                                          └───> JSONL recording
```

![Dash View dashboard](docs/dashboard.png)

## Features

- **44 standard telemetry tags** (`rpm`, `speed`, `gear`, temperatures, fuel,
  wheel speeds, acceleration, telltales, EV channels, …) plus vehicle-specific
  custom tags in `0x80..0xEF`.
- **Mock source** generating a synthetic drive cycle, for UI work without a car.
- **Replay source** for JSON Lines recordings, with loop and speed control.
- **BLE central** for the Arduino's tagged stream (experimental; protocol
  documented, firmware is a separate project).
- **Recording** of the tagged stream from any source.
- **Dashboard** with tach/shift lights, speed, gear, temperatures, fuel, pedals,
  torque, wheel speeds, steering, G-force, telltales and link statistics.
- **Debug overlay** (**D**) with the full tag catalog, ages, counts and the most
  recent values. Custom tags show up here.
- Unit tests for the codec, recorder/replay and vehicle state.

Not in this repository: the Arduino firmware, vehicle CAN maps (the G29 example
is documented for the firmware project), and raw CAN/SocketCAN transport.

## Requirements

- macOS 26+ (the Homebrew Qt 6.12 bottles used here require macOS 26; tested on
  macOS 26.6, Apple Silicon)
- Xcode Command Line Tools
- CMake 3.24+
- Qt 6.5+ with Qt Quick and Qt Bluetooth

```sh
brew install qtbase qtdeclarative qtshadertools qtconnectivity
```

Homebrew's Qt is split across kegs. `CMakePresets.json` points Qt at
`/opt/homebrew` (Apple Silicon); on Intel, replace it with `/usr/local`.

## Build and run

```sh
cmake --preset macos-debug
cmake --build --preset macos-debug
ctest --preset macos-debug
open build/debug/dash_view.app
```

Or pass options from the terminal:

```sh
./build/debug/dash_view.app/Contents/MacOS/dash_view
```

### Options

| Option | Description |
|---|---|
| `--source mock\|replay\|ble` | Telemetry source (default `mock`) |
| `--replay <file>` | JSON Lines recording to replay |
| `--loop` | Loop the replay |
| `--speed <factor>` | Replay speed (default `1.0`) |
| `--record <file>` | Record the current stream as JSON Lines |
| `--ble-name <substring>` | BLE device name filter (default `DashView`) |
| `--vehicle-name <name>` | Override the vehicle name in the UI |
| `--list-tags` | Print the standard tag catalog and exit |
| `--debug` | Open the telemetry debug overlay |
| `--fullscreen` | Start full screen |
| `--screenshot <file>` | Render the window to a PNG and exit |
| `--screenshot-delay <ms>` | Delay before the screenshot (default 2500) |

### Examples

```sh
# Demo
./build/debug/dash_view.app/Contents/MacOS/dash_view --fullscreen

# Record the demo, then replay it at 2x
./build/debug/dash_view.app/Contents/MacOS/dash_view \
  --source mock --record build/demo.jsonl
./build/debug/dash_view.app/Contents/MacOS/dash_view \
  --source replay --replay build/demo.jsonl --speed 2 --loop

# Live Arduino stream
./build/debug/dash_view.app/Contents/MacOS/dash_view \
  --source ble --ble-name DashView --record build/car.jsonl

# Inspect the tag catalog
./build/debug/dash_view.app/Contents/MacOS/dash_view --list-tags
```

### Controls

- **D** — telemetry debug overlay
- **F** — full screen
- **Esc** — close the debug overlay

## Debug overlay

![Telemetry debug overlay](docs/debug.png)

The left table lists every standard tag (plus any observed custom tag) with its
value, age and count. The right table is a live view of the most recent values in
arrival order. This is the fastest way to validate the Arduino's tag stream.

## How to collect from any vehicle

1. In the **Arduino firmware** (separate project), decode the vehicle's CAN map
   into physical values.
2. Map each value to a standard tag from
   [`docs/telemetry-protocol.md`](docs/telemetry-protocol.md); extras go in
   custom tags `0x80..0xEF`.
3. Send 6-byte records over the BLE service, 1–3 per notification.
4. Run Dash View with `--source ble --record <file>`.
5. Replay the recording later without the car.

Standard tags need no app change. Custom tags are recorded and visible in the
debug overlay; adding one to the main dashboard is an app change.

The G29/J29 example — raw PT-CAN map, wiring and quirks — is in
[`docs/vehicles/bmw-g29.md`](docs/vehicles/bmw-g29.md), as a reference for the
firmware side.

## Project layout

```
CMakeLists.txt / CMakePresets.json
cmake/Info.plist.in              # macOS bundle + Bluetooth usage strings
docs/
  telemetry-protocol.md          # BLE GATT + 6-byte tagged records + tag catalog
  replay-format.md               # JSONL recording format
  vehicles/bmw-g29.md            # example firmware-side CAN map and wiring
src/
  main.cpp                       # CLI, source wiring, recording, screenshots
  telemetry/
    signaltag.{h,cpp}            # tag catalog and metadata
    telemetryvalue.h             # {tag, value}
    telemetrysource.{h,cpp}      # source interface
    telemetrycodec.{h,cpp}       # 6-byte tagged-union codec
    mocktelemetrysource.*        # synthetic drive cycle
    replaytelemetrysource.*      # JSONL replay
    telemetryrecorder.*          # JSONL recording
    bletelemetrysource.*         # Qt Bluetooth central (app target only)
  vehicle/vehiclestate.*         # coalescing, staleness, stats, debug data
qml/
  Main.qml                       # window, shortcuts, stage scaling
  Dashboard.qml                  # dashboard layout
  DebugOverlay.qml               # tag/recent-value tables
  Theme.qml, *.qml               # gauges, bars, telltales, panels
tests/
  tst_telemetry.cpp              # Qt Test suite
```

The design is 1440×900 and is uniformly scaled to the window. The default window
is 1280×800; **F** toggles full screen.

## Data flow

```
Mock / Replay / BLE ──TelemetryValue{tag,value}──> VehicleState ──values──> QML
                                            └────> TelemetryRecorder (--record)
```

`VehicleState` coalesces updates to ~30 Hz for QML and drops each value after its
per-tag timeout. Missing values render as `—`. The dashboard layout is generic;
signals the vehicle does not provide simply stay empty.

## Documentation

- [`docs/telemetry-protocol.md`](docs/telemetry-protocol.md) — BLE service,
  record format, tag catalog, rates and firmware checklist.
- [`docs/replay-format.md`](docs/replay-format.md) — recording/replay JSONL.
- [`docs/vehicles/bmw-g29.md`](docs/vehicles/bmw-g29.md) — G29 example map,
  wiring, termination and known quirks.

## Credits

The G29/J29 PT-CAN map and install notes come from
[The SECRET Ingredient!](https://thesecretingredient.neocities.org/bmw/can/g29/).
This project is not affiliated with BMW.
