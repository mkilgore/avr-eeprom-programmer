ATMega328P EEPROM Programmer
============================

This is a simple EEPROM programmer, designed to program the AM29F040B chip.
This project is fully functional at this stage, and includes three distinct
parts:

1. board
  - This is the schematic and PCB layout for the programmer. It uses an
    ATMega328P along with two shift registers to talk to the AM29F040B. It also
    includes a PLCC32 socket for the AM29F040B, a few LEDs for indicating
    read/write, and a serial in/out for talking to the programmer.

1. driver
  - This is the code that runs on the ATMega328P chip and executes the read/write
    commands against the AM29F040B chip.

3. programmer
  - This sends commands to the driver over a serial connection and tells it to
    either read, write, or erase the chip.
