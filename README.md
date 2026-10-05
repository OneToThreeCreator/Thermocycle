# Thermocycle
Experimental stand firmware to test durability of coatings sprayed using HVoF or plasma torch.
Plasma Dynamics UrFU laboratory
The control system consists of SCADA system based on RapidSCADA, and STM32F40x MCU controlling stepper movement, positioning heating torch and air coolers. They communicate using Modbus line (either via RS485 transceiver, or via USB-CDC (manual modification required to Modbus lib initialization).
WIP