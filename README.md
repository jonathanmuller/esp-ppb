# ESP-PPB

<p align="center">
  <a href="images/five.jpg">
    <img src="images/five.jpg" alt="Five ESP-PPB nodes" width="420">
  </a>
</p>

ESP-PPB is a battery‑powered, wireless timing and CSI synchronization platform built on ESP32‑C3. It phase‑locks multiple nodes to a dedicated ESP‑PPB access point using Wi‑Fi FTM and a VCTCXO disciplined by dual DACs, enabling sub‑PPB clock alignment and near‑phase‑coherent CSI captures.

It targets research and applied systems where wireless, multi‑node phase synchronization is required without cables, and where CSI data must be captured and shipped over Wi‑Fi for post‑processing.

<details>
<summary><strong>Quick Facts</strong></summary>

- Wireless, battery‑powered, multi‑node synchronization
- Phase‑locked via Wi‑Fi FTM + VCTCXO + dual DAC
- Nanosecond‑class RX timestamps (IDF hack in repo)
- CSI data sent over Wi‑Fi for post‑processing
</details>

## Why ESP‑PPB
There is no off‑the‑shelf CSI synchronization solution for ESP32 that is wireless, battery‑powered, and remotely observable over Wi‑Fi. The closest public alternative (e.g., Espargos) relies on physical interconnects between devices and does not offer battery‑first, over‑the‑air collection. ESP‑PPB is designed to remove those constraints.

## Project Status
This project has been running for several years and is currently pre‑production: multiple PCB revisions have been built and validated. I am actively looking for feedback, collaborators, and early users in universities, labs, and industry.

If you want hardware, contact me at `jonatahn.muller12@gmail.com` or open a discussion. A larger batch is planned soon; early interest helps reserve boards at lower cost. The design is open hardware (see `schematics/`), but I recommend ordering boards from me if you are not experienced with PCB RF design and antenna tuning.

## Hardware Overview
- ESP32‑C3 + VCTCXO
- Dual DAC (coarse + fine) for oscillator discipline
- OLED screen for live accuracy readout
- Battery charger (battery not included)
- Extra GPIOs for additional sensors

## How It Works (High Level)
- One ESP‑PPB node acts as the AP/FTM responder.
- Slave nodes initiate FTM exchanges every few hundred milliseconds (configurable).
- Promiscuous RX timestamps and a small IDF hack enable nanosecond‑level RX timing.
- Each node estimates clock drift (PPB slope) and corrects its VCTCXO via dual DACs.
- Nodes become phase‑locked to the AP, then exchange CSI with a fast AP response to anchor phase.

## Synchronization Accuracy
- Theoretical phase accuracy: < 1 degree per CSI frame (based on 1 ppb sync + 1 ms AP response timing).
- Practical single‑frame accuracy observed: ~5 degrees without averaging, due to noise and physical effects.

## Deployment Model
- Minimum 3 pods: 1 master/AP + 2 slaves.
- Slaves broadcast CSI‑derived measurements over Wi‑Fi.
- Any listener can collect data; a simple ESP32 connected to a PC is provided as a reference receiver.
- Post‑processing runs on the PC for angle‑of‑arrival or multi‑node algorithms.

## What You Can Do With It
- Angle‑of‑arrival estimation
- Multi‑node phase‑coherent CSI capture
- MUSIC and other super‑resolution direction‑finding methods
- Distributed sensing with synchronized, battery‑powered nodes

## Quick Start
```bash
. $IDF_PATH/export.sh
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## Key Files
- `main/main.c`: entry point and role selection
- `main/helper_init.c`: Wi‑Fi init, CSI, promiscuous mode
- `main/perf.c`: FTM table, PPB slope, DAC correction
- `main/i2c_helper.c`: OLED + DAC + I2C utilities

## Licensing
- Software/firmware: `GPL-3.0` (see `LICENSE`)
- Hardware design files: `CC-BY-NC-SA-4.0` (see `HARDWARE_LICENSE`)

## Get Involved
I’m actively looking for:
- Feedback on architecture and calibration
- Researchers who want to test in real deployments
- Ideas for new sensing and synchronization workflows

If you have suggestions or want to collaborate, open a discussion or email me.

<details>
<summary><strong>FAQ</strong></summary>

- **Is this open source hardware?**
- The hardware license is non‑commercial (`CC-BY-NC-SA-4.0`), so it is source‑available but not OSHWA‑compliant open hardware.

- **Can I build my own boards?**
- Yes. The schematics and design files are in `schematics/`. If you are not experienced with RF PCB design and antenna tuning, I recommend ordering from me.

- **Do I need a special access point?**
- Yes. The AP must be an ESP‑PPB node acting as the FTM responder.

- **What’s required to collect data?**
- Any listener can receive the broadcast data. A reference ESP32 logger connected to a PC is suggested for convenience.

</details>
