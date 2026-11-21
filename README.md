# Dash View

A Qt 6 / QML dashboard that consumes and records the tagged telemetry stream
from **dash-connector**.

Dash View has no vehicle-specific code or configuration. dash-connector owns the
car-specific CAN decoding and sends the **same tagged structure/protocol for every
car**; the dashboard simply displays whichever standard tags are present.

```
dash-connector ──tagged BLE values──> Dash View ──> display
                                              └──> JSONL recording
```

![Dash View dashboard](docs/dashboard.png)

## Features

- **44 standard telemetry tags** (`rpm`, `speed`, `gear`, temperatures, fuel,
  wheel speeds, acceleration, telltales, EV channels, …) plus custom tags in
  `0x80..0xEF`.
- **Mock source** with a synthetic drive cycle for UI work without a car.
- **Replay source** for JSON Lines recordings, with loop and speed control.
- **BLE central** for dash-connector's tagged stream (experimental; the connector
  is a separate project).
- **Recording** of the tagged stream from any source.
- **Dashboard** with tach/shift lights, speed, gear, temperatures, fuel, pedals,
  torque, wheel speeds, steering, G-force, telltales and link statistics.
- **Debug overlay** (**D**) with the full tag catalog, ages, counts and recent
  values.
- Unit tests for the codec, recorder/replay, vehicle state and mock source.

Not in this repository: dash-connector and the vehicle CAN maps. Those live in
the dash-connector project.

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

# Live dash-connector stream
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
arrival order. This is the fastest way to validate dash-connector's tag stream.

## The dash-connector contract

- dash-connector sends a fixed tagged structure; see
  [`docs/dash-connector-protocol.md`](docs/dash-connector-protocol.md).
- Every car uses the same standard tags. Signals a car does not provide are
  simply absent and render as `—`.
- Extra vehicle-specific channels go in custom tags `0x80..0xEF`; they are
  recorded and shown in the debug overlay.
- Adding a car is a **dash-connector** change, not a Dash View change.

## Project layout

```
CMakeLists.txt / CMakePresets.json
cmake/Info.plist.in              # macOS bundle + Bluetooth usage strings
docs/
  dash-connector-protocol.md     # BLE GATT + 6-byte tagged records + tag catalog
  replay-format.md               # JSONL recording format
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
per-tag timeout. Missing values render as `—`.

## Documentation

- [`docs/dash-connector-protocol.md`](docs/dash-connector-protocol.md) — BLE
  service, 6-byte record format, tag catalog, rates and the connector checklist.
- [`docs/replay-format.md`](docs/replay-format.md) — recording/replay JSONL.
