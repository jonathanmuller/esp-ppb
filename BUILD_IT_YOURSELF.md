# Build It Yourself

This guide targets labs, hobbyists, and anyone who wants to build and run ESP-PPB from source and production files.

## What You Need

- At least 3 ESP-PPB boards (recommended: 5 nodes + 1 listener)
- USB-C cable(s)
- Optional: LiPo battery (PH2.0 - 2P), see the battery note below
- No special tools required

## Software Prerequisites

- ESP-IDF 6.0

## Production Files

All manufacturing outputs are available here:

- BOM: `schematics/production/ESP-PPB_BOM.csv`
- Gerbers: `schematics/production/ESP-PPB_Gerbers.zip`
- Pick-and-place: `schematics/production/ESP-PPB_PickAndPlace.xlsx`
- Schematic PDF: `schematics/production/ESP-PPB_Schematic.pdf`
- Schematic DXF: `schematics/production/ESP-PPB_Schematic_DXF.zip`
- Netlist: `schematics/production/ESP-PPB_Netlist.tel`
- Altium source: `schematics/production/ESP-PPB_Altium_Source.zip`
- PADS archive: `schematics/production/ESP-PPB_PADS_Archive.zip`

## Power Options

- USB-C works without a battery.
- If you use a battery, verify polarity. The `bat+` label on the board marks the positive side.
- Set the power switch to `battery` or `5V` depending on your power source.

## Flashing (Standard Espressif Flow)

Use the standard ESP-IDF flow. The board exposes `BOOT` and `FLASH` buttons.

```bash
. $IDF_PATH/export.sh
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## Role Assignment (MAC-Based)

On first boot, the device prints its STA MAC address in the serial console. Use that MAC to assign the role
by editing the constants in:

- `main/helper.h`

Set `MAC_STA_LEFT`, `MAC_STA_RIGHT`, `MAC_STA_TOP`, `MAC_STA_BOTTOM`, and `MAC_STA_MIDDLE` as needed.

## Recommended Workflow

Suggested setup:

- 5 ESP-PPB nodes: `LEFT`, `RIGHT`, `TOP`, `BOTTOM`, `MIDDLE`
- 1 listener node (any ESP32 is fine, ESP32-C3 is prefered for simplicity of code/compilation) to receive broadcasts

The listener is the `default` role in [`main/main.c`](main/main.c) and prints received frames to the console.
The current output format is implemented in [`main/perf.c`](main/perf.c) (`print_now_recv`), including
per-subcarrier angle dumps. The CSI frame format reference is:

https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/wifi.html#wi-fi-channel-state-information

## What You Should See

On boot:

- Serial logs with the MAC address
- OLED status updates

When running:

- Slaves sync and begin broadcasting CSI data
- Listener prints frames in the console

## Calibration

No calibration is required for basic operation.

## Prototype Status

This design has been tested with 10+ nodes.

## Troubleshooting

- LED is red: set the power switch to `USB` if you do not have a battery connected.
- ESP-PPB acts as emitter: its MAC address was not assigned to a role in [`main/helper.h`](main/helper.h).
- Angle-of-arrival looks random: you did not account for the sync frame; all angles are relative to the sync frame.
