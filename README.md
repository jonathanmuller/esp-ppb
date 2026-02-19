# Timesync (ESP32-C3)

This project measures Wi‑Fi timing drift between devices using FTM (Fine Timing Measurement), estimates clock error in PPB, and applies corrections to a TCXO via dual DACs. It also captures CSI snapshots for diagnostics and shows live results on a small OLED.

## What It’s For

- Measure relative clock drift between ESP32‑C3 nodes.
- Apply hardware correction (TCXO tuning) based on PPB slope.
- Provide quick on‑device feedback (OLED) and debug logs.

## How It Works (High Level)

- **FTM sniffing**: FTM action frames are captured in promiscuous mode.
- **PPB estimation**: A simple linear regression estimates drift from `(recv_ts − tod)`.
- **Correction**: Coarse and fine DACs adjust the TCXO.
- **CSI ping‑pong**: ESP‑NOW frames trigger CSI capture for RF diagnostics.

## Roles

Roles are selected by the device’s STA MAC (see `main/helper.h`).

- **FTM initiators**: `LEFT`, `RIGHT`, `TOP`, `BOTTOM`
- **FTM responder/AP**: `MIDDLE`
- **Default client**: any other MAC

## OLED Output

Displayed lines:

- `PPB: %+0.1f`
- `Pkt: %d`

## Hardware

- **OLED** (I2C `0x3C`, HS91L02W2C01)
- **DACs** (MCP4725 at `0x60` and `0x61`)
- **Battery sense** on GPIO3 (ADC) with 100k/100k divider
- **5V present** on GPIO20 (U0RXD)

## Build and Flash

```bash
. $IDF_PATH/export.sh
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## Important Files

- `main/main.c`: entry point and role selection
- `main/helper_init.c`: Wi‑Fi init and CSI/promiscuous setup
- `main/perf.c`: FTM table, slope calculation, DAC correction
- `main/i2c_helper.c`: I2C, DAC, OLED drawing
