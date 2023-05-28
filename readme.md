Arduino project for recicling pet-bottles into filament for 3d printer

Components:
    - Arduino (UNO|Nano|ProMini)
    - OLED display 128x64 (SSD1306)
    - Encoder with button
    - Termistor 3950 100kOhm
    - Step engine driver DRV8825
    - Relay

In project was used hardware interrupts, interrupts by timer, simple temperatre filtring.

UserInterface localised ukrainian language.

Software version 0.1 suports basic actions such as
    choose engine rotate direction
    choose engine speed (by rpm, step 10)
    smooth start & stopof engine
    choose temperature for heating
    separate settigs for bottle cut & strerch plastic tape throught extruder
    EEPROM configs storage

Usage instructions:
For change rotation direction set cursor to "Двигун", then press button and rotate encoder
For change rotation speed set cursor to "Швидкість", then press button and rotate encoder
For dst-temperature set cursor to "Швидкість", then press button and rotate encoder
*all other menu-entaris may be acivated by siingle press