# Schematics Folder Guide

This folder holds the board documentation assets. The goal is simple: give you everything you need to understand the hardware without opening the CAD tools.

## Quick Start

If you are new:

1. Start with `pcb.png` to see the full board layout.
2. Then open `esp32_c3.png` to understand the MCU wiring.
3. Use `power_switch.png`, `LDO.png`, and `usb_c.png` to follow power flow.
4. Check `battery_charge.png` and `voltage_dividers.png` to understand battery measurement.
5. Finally look at `oled.png` and `I2c.png` for the display and I2C wiring.

## Usage

<details>
<summary><strong>Board Layout</strong></summary>

Overall board layout. Use this to locate components and understand where connectors, buttons, and signals physically sit.

![PCB](./pcb.png)
</details>

<details>
<summary><strong>ESP32-C3 Microcontroller</strong></summary>

Microcontroller wiring. This is the heart of the design. It shows which GPIOs are used, how the crystal is connected, and where power and reset lines go. Use it to map firmware pins to hardware signals.

![ESP32-C3](./esp32_c3.png)
</details>

<details>
<summary><strong>Exposed IO / Test Points</strong></summary>

Exposed headers or test points. Use this to find accessible signals for probing, flashing, or expansion.

![Exposed IO](./exposed_io.png)
</details>

<details>
<summary><strong>USB-C Connector</strong></summary>

USB-C connector wiring. Use this to see how 5V enters the board and what pins are connected. Helpful when debugging power input or USB wiring.

![USB-C](./usb_c.png)
</details>

<details>
<summary><strong>Power Switch</strong></summary>

Power switch circuit. Follow how the board enables/disables power and which nets are switched. Useful if the device does not boot or power cycles.

![Power Switch](./power_switch.png)
</details>

<details>
<summary><strong>LDO Regulator</strong></summary>

Voltage regulator (LDO). This shows how the board converts input power down to the logic voltage. Use this when verifying power rails or checking for voltage drop.

![LDO](./LDO.png)
</details>

<details>
<summary><strong>Battery Charger</strong></summary>

Battery charging circuit. Useful if you are debugging charging, battery safety, or powering the board from a LiPo.

![Battery Charge](./battery_charge.png)
</details>

<details>
<summary><strong>Voltage Dividers</strong></summary>

Voltage dividers used for measurement (for example, battery sense). This is where the ADC scaling is defined, so it directly impacts calculated battery voltage.

![Voltage Dividers](./voltage_dividers.png)
</details>

<details>
<summary><strong>I2C Bus</strong></summary>

I2C bus wiring. Shows which devices live on the I2C bus and how they are connected. Use this when debugging OLED/DAC access or bus contention.

![I2C](./I2c.png)
</details>

<details>
<summary><strong>TCXO and Dual DAC</strong></summary>

Clock and DAC circuitry. This is relevant if you are working on timing precision or DAC calibration. It shows how the reference clock is tuned and how corrections are applied.

![TCXO + Dual DAC](./tcxo_dual_dac.png)
</details>

<details>
<summary><strong>OLED Display</strong></summary>

OLED display wiring. Use this when debugging the display or changing screen wiring. Shows power, reset, and I2C pins.

![OLED](./oled.png)
</details>

<details>
<summary><strong>3D Board Model</strong></summary>

- `3d_view.obj`: 3D geometry.
- `3d_view.mtl`: material definition.

You can open the OBJ in most 3D viewers (Blender, FreeCAD, etc.) to see the assembled board in 3D.
</details>

<details>
<summary><strong>Altium Sources</strong></summary>

There are two sources here:

- `altium.zip`: full Altium project archive. This is the source of truth.

If you need to edit the schematic or PCB:

1. Prefer `altium.zip`.
2. Unzip it and open the project in Altium Designer.

</details>
