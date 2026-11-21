# Hardware notes — tapping the G29 PT-CAN

**This build does not talk to the car yet.** It runs on mock data. These notes are
for the separate MKR firmware project and for wiring the tap safely.

## PT-CAN is not the OBD-II port

The dashboard map is for the **PT-CAN** (powertrain CAN, 500 kbit/s). The OBD-II
socket on a G29 is on the **D-CAN** and carries diagnostic traffic, not the
`0x0A5`, `0x254`, `0x3F9`, etc. broadcasts used here. Connecting the MKR CAN
shield to the OBD socket will not produce this data.

Reference: [RaceCapture integration / PT-CAN install](https://thesecretingredient.neocities.org/racecapture/).

## Tap location

The reference page taps the PT-CAN in the **passenger outer footwell**, behind
the plastic kick panel:

- **CAN HI** — yellow/white
- **CAN LO** — yellow/black

Use a proper tap (the reference uses Posi-Taps), keep the twisted pair as short
as practical, and secure the harness so it cannot rub or pull on the connector.

## Termination

A CAN bus is terminated at both physical ends, normally with 120 Ω each. A tap
is a stub, not a new end. **Do not add a third 120 Ω termination in the middle of
the bus.** Check the MKR CAN shield schematic: if it has an onboard 120 Ω
resistor, remove/disable it for a mid-bus tap. Measure across CAN H/L with the
bus off and the tap disconnected; a healthy terminated bus reads about 60 Ω.

## Power and ground

- The MKR WiFi 1010 can be powered over USB during development, or from the
  shield's buck converter from a fused 12 V supply in the car.
- Keep a common ground between the shield, the MKR and the car chassis.
- Do not backfeed the car through the USB port, and do not power the board from
  two sources at once unless the shield is designed for it.
- Fuse any permanent 12 V feed and use ignition-switched power if the install
  should not drain the battery.

## CAN controller

The Arduino MKR CAN Shield uses:

- **MCP2515** CAN controller over SPI
- **TJA1049** high-speed CAN transceiver
- MKR header pins **CS = D3**, **INT = D7**
- 500 kbit/s on the G29 PT-CAN (confirm the shield's crystal, normally 16 MHz)

Run the MCP2515 in **listen-only** mode. Sniffing the bus must not transmit
application frames. Listen-only also avoids the CAN ACK bit that a normal-mode
controller would drive. Passive listening is the goal; the dashboard is not a
bus master.

The firmware belongs to a separate project. The proposed Bluetooth transport and
record format are in [`ble-protocol.md`](ble-protocol.md).

## Bluetooth

The MKR WiFi 1010's NINA-W102 module is Bluetooth Low Energy. The Mac is the BLE
central (Qt Bluetooth); the MKR is the peripheral. The reference firmware project
should advertise the custom service, filter the mapped CAN identifiers, and send
rate-limited records over a notify characteristic.

## Safety

- Do not transmit on the PT-CAN. Other modules own that bus.
- Do not disconnect or short the CAN pair while the car is awake.
- Secure all wiring away from pedals, steering, airbags and moving parts.
- Test with the car stationary and the wheels chocked before driving.
- This is a hobby project; you are responsible for your own installation.
