# ESP-PPB

![Platform](https://img.shields.io/badge/Platform-ESP32--C3-green)
![Status](https://img.shields.io/badge/Status-Pre--Production-orange)
![Crowd Supply](https://img.shields.io/badge/Crowd_Supply-Coming_Soon-purple)

<p align="center">
  <a href="images/five.jpg">
    <img src="images/five.jpg" alt="Five ESP-PPB nodes" width="520">
  </a>
</p>

**ESP-PPB** is the first wireless, battery-powered, phase-coherent CSI synchronization platform for ESP32. It phase-locks any number of nodes over the air using Wi-Fi FTM and a VCTCXO disciplined by dual DACs, achieving sub-PPB clock alignment and near-phase-coherent CSI captures — no cables, no wired backhaul, no tethered power.

Drop nodes wherever you need them, power them on, and collect synchronized CSI data on your laptop over Wi-Fi.

> **Looking for hardware?** A Crowd Supply campaign is planned. In the meantime, early boards are available directly — see [Get Hardware](#get-hardware) below.

---

## Why ESP-PPB

Existing Wi-Fi CSI platforms either require cables between antennas, need a wired connection to a PC, or cannot synchronize phase across devices. ESP-PPB removes all three constraints:

|                                        | **ESP-PPB**                  | **ESPARGOS**         | **Intel 5300 CSI Tool** | **Atheros CSI Tool**      |
|----------------------------------------|------------------------------|----------------------|-------------------------|---------------------------|
| **Wireless (no cables between nodes)** | Yes                          | No (coax + Ethernet) | No (PCIe in PC)         | Partial (OpenWRT routers) |
| **Remote data collection**             | Yes (battery powered +Wi-Fi) | No (Ethernet)        | No (local)              | Partial (router)          |
| **Max synced nodes**                   | Unlimited                    | ~4 arrays documented | N/A                     | N/A                       |
| **Antennas per node**                  | 1                            | 8 (2x4 array)        | 3 (3x3 MIMO)            | Up to 3 (3x3 MIMO)        |
| **Open source firmware**               | Yes                          | Yes                  | No (closed binary)      | Yes                       |
| **Actively maintained**                | Yes                          | Yes                  | No (discontinued ~2011) | Limited                   |
| **Cost per node**                      | ~$50-80                      | Not available        | Discontinued            | ~$90-145 (router)         |

**In short:** ESP-PPB is the only solution that is simultaneously wireless, battery-powered, phase-synchronized, and remotely observable — with no upper limit on the number of synced nodes.

---

## What You Can Do With It

- **Angle-of-arrival estimation** — place nodes around a room, triangulate sources
- **MUSIC / ESPRIT** and other super-resolution direction-finding algorithms
- **Multi-node phase-coherent CSI capture** — distributed virtual array
- **Distributed wireless sensing** — synchronized, cable-free, battery-powered nodes
- **Indoor localization research** — deploy and relocate freely without cable constraints

---

## How It Works

```
┌──────────┐                     ┌──────────┐
│  Slave 1 │ ◄──────────────────►│          │
│ (VCTCXO) │─────┐          FTM  │          │
└──────────┘     │               │  Master  │
                 │               │          │
                 │               │   (AP)   │
┌──────────┐◄───────────────────►│          │
│          │     |          FTM  └──────────┘
│  Slave 2 │─────┤            
│ (VCTCXO) │     │               
└──────────┘     │
                 │  CSI data (Wi-Fi broadcast)
  ...more...─────┤
                 │
            ┌────▼─────┐
            │ Listener │  ← any ESP32 + PC
            │   (PC)   │
            └──────────┘
```

1. **One node acts as the AP / FTM responder** (the master clock).
2. Slave nodes initiate FTM exchanges every few hundred milliseconds (configurable).
3. A small IDF hack enables **nanosecond-level RX timestamps** via promiscuous mode.
4. Each slave estimates its clock drift (PPB slope) and corrects its **VCTCXO via dual DACs** (coarse + fine).
5. Once phase-locked, slaves exchange CSI with the AP and **broadcast results over Wi-Fi**.
6. Any listener (a cheap ESP32 connected to a PC) collects the data for post-processing.

---

## Synchronization Accuracy

| Metric                          | Value                                   |
|---------------------------------|-----------------------------------------|
| Clock alignment                 | Sub-PPB (parts per billion)             |
| Theoretical phase accuracy      | < 1 degree per CSI frame                |
| Practical single-frame accuracy | ~5 degrees (without averaging)          |
| Time to 10 PPB lock             | Seconds                                 |
| Time to < 1 PPB lock            | Minutes (thermal stabilization needed)  |
| Battery life                    | hours to days (depends on battery size) |

---

## Hardware

<p align="center">
  <a href="images/single.jpg">
    <img src="images/single.jpg" alt="Single ESP-PPB node" width="300">
  </a>
  <a href="images/five.jpg">
    <img src="images/five.jpg" alt="Five ESP-PPB nodes" width="300">
  </a>
</p>

- **Compact** — under 4 cm x 4 cm, 5+ PCB revisions refined
- **ESP32-C3** with custom RF antenna tuning (2.4 GHz)
- **VCTCXO** (voltage-controlled temperature-compensated crystal oscillator)
- **Dual DAC** — coarse + fine control for oscillator discipline
- **OLED display** — live accuracy and status readout
- **LiPo battery charger** (USB-C, battery not included)
- **6 exposed GPIOs** — connect your own sensors, actuators, or peripherals

Full schematics, PCB layout, and 3D board model are in [`schematics/`](schematics/).

---

## Deployment

**Minimum setup:** 1 master (AP) + 2 slaves = 3 nodes.

**Typical setup:** 1 master + 4 slaves, with a listener ESP32 connected to a laptop.

Nodes auto-detect their role based on MAC address (configurable in firmware). Power them on and they synchronize automatically.

---

## Quick Start

### Prerequisites

- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/) v5.x or v6.0
- USB-C cable for flashing

### Build and flash

```bash
. $IDF_PATH/export.sh
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### Key files

| File                 | Purpose                                |
|----------------------|----------------------------------------|
| `main/main.c`        | Entry point and role selection         |
| `main/helper_init.c` | Wi-Fi init, CSI, promiscuous mode      |
| `main/perf.c`        | FTM table, PPB slope, DAC correction   |
| `main/i2c_helper.c`  | OLED + DAC + I2C utilities             |
| `main/constant.h`    | Channel, SSID, and protocol constants  |
| `hack_struct.patch`  | IDF patch for nanosecond RX timestamps |

---

## Get Hardware

**Early boards are available now.** A larger batch and a Crowd Supply campaign are planned — early interest helps reserve boards at lower cost.

Contact: **`jonathan.muller12@gmail.com`** or [open a discussion](../../discussions).

The design files are in [`schematics/`](schematics/) if you want to build your own, but I recommend ordering assembled boards unless you are experienced with RF PCB design and antenna tuning.

---

<!-- Uncomment when video is ready
## Demo

[![ESP-PPB Demo](https://img.youtube.com/vi/VIDEO_ID/maxresdefault.jpg)](https://www.youtube.com/watch?v=VIDEO_ID)
-->

## Licensing

| Component             | License                             |
|-----------------------|-------------------------------------|
| Firmware / software   | [GPL-3.0](LICENSE)                  |
| Hardware design files | [CC-BY-NC-SA-4.0](HARDWARE_LICENSE) |

---

## Get Involved

> **This project thrives on early feedback.** Whether you're a researcher, engineer, or hobbyist — your input shapes the next revision.

| # | How to contribute |
|---|---|
| 1 | **Try it** — request a board and test in your environment |
| 2 | **Report** — share results, bugs, or calibration observations |
| 3 | **Suggest** — propose new use cases or features |
| 4 | **Collaborate** — co-author research, co-develop algorithms |

[Open a discussion](../../discussions) or email **`jonathan.muller12@gmail.com`**.

---

## Roadmap

### Progress

- [x] First prototype
- [x] Prototype validation
- [x] Multi-revision PCB refinement (5+ revisions)
- [x] Sub-PPB synchronization demonstrated
- [x] Multi-node deployment tested (5+ nodes)
- [x] Open source firmware release
- [ ] **Early production** ← *you are here*
- [ ] Crowd Supply campaign
- [ ] Python post-processing examples (AoA, MUSIC)
- [ ] Extended documentation and tutorials

---

## Cite

If you use ESP-PPB in academic work, please cite:

```bibtex
@misc{muller2025espppb,
  author       = {Jonathan Muller},
  title        = {{ESP-PPB}: Wireless Battery-Powered Phase-Coherent {CSI} Synchronization Platform},
  year         = {2025},
  howpublished = {\url{https://github.com/jonathanmuller/esp-ppb}},
}
```

---

<details open>
<summary><strong>FAQ</strong></summary>

**Do I need a special access point?**
An ESP-PPB node as AP gives the best results (built-in FTM sync). A regular Wi-Fi router also works, but you'll need an external ESP32 (doesn't have to be ESP-PPB) to send a sync frame alongside each CSI frame.

**What's required to collect data?**
Any Wi-Fi listener can receive the broadcast data. A reference ESP32 logger connected to a PC is provided for convenience.

**How many nodes can I synchronize?**
There is no hard limit. The system has been tested with more than 5 nodes. Add as many slaves as you need.

**What ESP-IDF version do I need?**
ESP-IDF v5.x works. v6.0 is also supported.

</details>

---

<details open>
<summary><strong>Code Architecture</strong></summary>

Each node selects its role at boot based on its MAC address (`main/main.c`). The role determines which callbacks and loop function are registered.

### Roles

| Role | MAC match | Wi-Fi mode | Description |
|---|---|---|---|
| **Slave (FTM)** | `LEFT`, `RIGHT`, `TOP`, `BOTTOM` | Station | Syncs to master via FTM, captures and broadcasts CSI |
| **Master (AP)** | `MIDDLE` | Access Point | Runs FTM responder, responds to CSI pings |
| **Default client** | Any other MAC | Station | Receives and logs broadcast data from slaves (listener / PC bridge) |

### Callbacks and main loops

| Function | Registered as | Role | Trigger | What it does |
|---|---|---|---|---|
| `promi_ftm_cb` | Promiscuous RX callback | Slave | Every received management frame | Extracts nanosecond RX timestamps from FTM frames for clock drift estimation |
| `csi_send_summary` | CSI RX callback | Slave | Every CSI frame received | Packages CSI + timing data and broadcasts it over Wi-Fi |
| `infinite_ftm` | Main loop | Slave | Runs continuously | Initiates FTM exchanges with the AP, estimates PPB drift, corrects VCTCXO via dual DACs |
| `csi_ping_pong` | CSI RX callback | Master | CSI frame from a slave | Immediately responds with a CSI frame back (anchors phase for the slave) |
| `print_now_recv` | ESP-NOW RX callback | Default | ESP-NOW packet received | Logs received data (used by the listener / PC bridge) |
| `simple_send_cb` | ESP-NOW TX callback | Default | After sending an ESP-NOW packet | Logs TX rate for debugging |
| `infinite_send` | Main loop | Default | Runs continuously | Sends ESP-NOW packets in a loop (test / relay mode) |

### Boot sequence (`app_main`)

```
1. Print MAC address
2. Init GPIO9 interrupt (button)
3. Init I2C → OLED → DACs (reset VCTCXO)
4. Detect role from MAC address
5. Init Wi-Fi (AP or Station)
6. Register callbacks (promiscuous, CSI, ESP-NOW)
7. Enter main loop (if any)
```

</details>
