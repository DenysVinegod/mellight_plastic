# Arduino Project for Recycling PET Bottles into Filament for 3D Printer

## Components
- Arduino (UNO | Nano | Pro Mini)
- OLED display 128x64 (SSD1306)
- Encoder with button
- Thermistor 3950 100kOhm
- Stepper motor driver DRV8825
- Relay

## Features
In this project, hardware interrupts, timer interrupts, and simple temperature filtering were used.

The user interface is localized in the Ukrainian language.

### Software Version 0.1
Supports basic actions such as:
- Choosing engine rotation direction
- Choosing engine speed (by RPM, step 10)
- Smooth start & stop of the engine
- Choosing temperature for heating
- Separate settings for bottle cutting & stretching plastic tape through extruder
- EEPROM configuration storage

## Usage Instructions
- To change the rotation direction, set the cursor to "Двигун", then press the button and rotate the encoder.
- To change the rotation speed, set the cursor to "Швидкість", then press the button and rotate the encoder.
- For target temperature, set the cursor to "Температура", then press the button and rotate the encoder.
- All other menu entries may be activated by a single press.

## Menu Options in Ukrainian
- **Двигун** (Engine)
- **Швидкість** (Speed)
- **Температура** (Temperature)
- **Всі інші параметри можуть бути активовані одноразовим натисканням** (All other parameters can be activated by a single press)
