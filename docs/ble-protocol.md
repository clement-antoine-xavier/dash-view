# Proposed BLE protocol (MKR ⇄ Dash View)

Status: **specification only**. The firmware is a separate project and the Qt app
currently has no BLE transport. This document is the contract to implement on
both sides.

## Roles

- **Peripheral:** Arduino MKR WiFi 1010 + MKR CAN Shield (500 kbit/s PT-CAN).
- **Central:** Dash View on macOS, using Qt Bluetooth (`QLowEnergyController`).

No pairing/bonding is required for v1. The characteristic is notify-only and
carries broadcast vehicle data.

## GATT

| Item | UUID |
|---|---|
| Service | `6b8d0001-1f2e-4a3b-9c4d-5e6f7a8b9c0d` |
| CAN stream (notify) | `6b8d0002-1f2e-4a3b-9c4d-5e6f7a8b9c0d` |
| Commands (write, optional) | `6b8d0003-1f2e-4a3b-9c4d-5e6f7a8b9c0d` |

- Device name: `BMW-Dash`
- Advertise the service UUID so the central can filter during discovery.
- CAN stream characteristic: 20-byte value, `BLENotify`.
- Commands characteristic: unused in v1; reserve for rate/profile changes.

ArduinoBLE's usable notification payload is effectively **20 bytes** (ATT MTU
23; do not assume the central negotiates a larger MTU). The format is designed
around that limit.

## CAN record format

Each notification contains **one or two 10-byte records**. Treat the payload as a
stream of records; the receiver must not assume that a notification is exactly
one frame.

```
byte 0: ((id >> 8) & 0x07) << 4 | (dlc & 0x0F)
byte 1:   id & 0xFF
bytes 2..9: data[0..7]     // always 8 bytes on the wire; unused bytes are 0
```

- `id` is the 11-bit classical CAN identifier.
- `dlc` is 0…8. All mapped G29 messages carry 8 data bytes.
- Bit 7 of byte 0 is reserved (0) for a future extended-ID flag.
- Maximum notification = 2 records = 20 bytes.
- No checksum: BLE's link layer checks integrity. A malformed `id > 0x7FF` or
  `dlc > 8` record is dropped by the app.

Example, record for `0x0A5` with DLC 8:

```
08 A5 00 00 73 B7 4B 0D 68 00
^  ^  ^^^^^^^^^^^^^^^^^^^^^^^
|  |  8 data bytes
|  id low byte
dlc=8, id high nibble=0
```

## Firmware responsibilities

1. Configure the MCP2515 for **500 kbit/s, listen-only**, and never transmit.
2. Accept only the mapped identifiers; drop everything else as early as possible.
3. Maintain a small per-ID rate limit (table below). Update the CAN controller's
   receive buffers continuously; do not let a BLE write block the CAN path.
4. Buffer rate-limited frames. When two records are pending (or a short timeout
   expires with one), send a notification.
5. Expose a counter/log for MCP2515 RX overflow and dropped frames.
6. Never block on a BLE connection; when no central is connected, keep the CAN
   side alive and discard/aggregate.

Suggested default profile (≈59 records/s, well within 2 records per 15–30 ms
connection event):

| CAN ID | Contents | Rate |
|---|---|---|
| 0x0A5 | RPM, engine torque, gearbox torque | 10 Hz |
| 0x0D9 | accelerator, throttle | 10 Hz |
| 0x254 | 4 wheel speeds | 10 Hz |
| 0x1A1 | vehicle speed | 5 Hz |
| 0x301 | steering | 5 Hz |
| 0x0EF | brake | 5 Hz |
| 0x3F9 | coolant, oil, max RPM, gear | 5 Hz |
| 0x173 | ABS/ASC/brake status | 1 Hz |
| 0x199/0x19A | acceleration X/Y | 1 Hz each |
| 0x19F | yaw rate | 1 Hz |
| 0x281 | battery voltage | 1 Hz |
| 0x2C4 | fuel raw | 1 Hz |
| 0x2CA | ambient temperature | 0.5 Hz |
| 0x330 | fuel used/lamp/range | 1 Hz |
| 0x1EE | high beam | 1 Hz |
| 0x1F6 | indicators | 1 Hz |

RPM at 10 Hz is acceptable because the QML needle interpolates; raise it to
15 Hz only if the connection interval allows it. If the central reports drops,
lower the rates before adding compression.

## App responsibilities (future work)

1. Scan for the device name/service UUID; connect with `QLowEnergyController`.
2. Discover the service, enable notifications on the stream characteristic by
   writing the CCCD (`CCCDEnableNotification`).
3. Buffer each notification and split it into 10-byte records. Validate
   `id <= 0x7FF`, `dlc <= 8`, and that at least 10 bytes are available.
4. Emit `CanFrame{id, dlc, data, timestampMs}` into the existing `CanSource`
   path. No decoder changes are needed.
5. Auto-reconnect with backoff, remember the last device, and show link state.
6. Expose a debug counter for notifications received / malformed records.

The app's timeout logic already tolerates a lost record: individual values
disappear after their per-signal timeout and render as `—`.

## Versioning

No version byte in v1. If the format must change, use the commands
characteristic to negotiate a revision, or change the service UUID. Keep the
record layout stable: it is intentionally small and transport-agnostic.
