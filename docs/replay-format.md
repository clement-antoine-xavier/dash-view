# Recording and replay format

Dash View records the **tagged telemetry stream**, not raw CAN. The format is
JSON Lines (`.jsonl`), one object per line:

```json
{"source":"DEMO","type":"meta","vehicle":"Demo vehicle","version":1}
{"name":"rpm","t":50,"tag":1,"v":858}
{"name":"speed","t":50,"tag":2,"v":0}
```

## Meta line

Written at recording start and whenever the vehicle name changes:

| Field | Meaning |
|---|---|
| `type` | `"meta"` |
| `version` | format version, currently `1` |
| `vehicle` | vehicle name shown in the UI |
| `source` | `DEMO`, `REPLAY` or `BLE` |

## Value line

| Field | Meaning |
|---|---|
| `t` | milliseconds since recording start |
| `tag` | numeric telemetry tag (see [`telemetry-protocol.md`](telemetry-protocol.md)) |
| `v` | physical value |
| `name` | optional tag key, for human readability |

The recorder stamps values with their arrival time. Values that arrive together
keep the same `t`; gaps in the recording are preserved on replay.

## Record

```sh
# Record the demo stream
./build/debug/dash_view.app/Contents/MacOS/dash_view \
  --source mock --record recordings/demo.jsonl

# Record the live Arduino stream
./build/debug/dash_view.app/Contents/MacOS/dash_view \
  --source ble --record recordings/car.jsonl

# Record a replay (useful for trimming / converting a capture)
./build/debug/dash_view.app/Contents/MacOS/dash_view \
  --source replay --replay recordings/car.jsonl --record recordings/copy.jsonl
```

`--record` works with every source. The file is flushed periodically and on quit.

## Replay

```sh
./build/debug/dash_view.app/Contents/MacOS/dash_view \
  --source replay --replay recordings/car.jsonl

# Loop and play at 2x
./build/debug/dash_view.app/Contents/MacOS/dash_view \
  --source replay --replay recordings/car.jsonl --loop --speed 2
```

If `--replay` is supplied without `--source`, the replay source is selected
automatically. At the end of a non-looping replay the source stops; values time
out and the dashboard shows `—`.

## Custom tags

Any tag can be recorded, including vehicle-specific tags in `0x80..0xEF`. The
debug overlay (**D**) shows them with a `Custom 0xNN` label. This is how you can
collect arbitrary extra channels from a car without a dashboard change; add a
widget later if the channel deserves one.

## Compatibility

- Unknown JSON fields are ignored.
- Invalid value lines are skipped; they do not abort the replay.
- A missing meta line falls back to the file name as the vehicle name.
- The format is deliberately transport-agnostic: the same file can be replayed
  on any machine without the car or the Arduino.
