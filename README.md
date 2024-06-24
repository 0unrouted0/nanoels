This software and instructions are [provided as is](LICENSE), without warranty of any kind.

See [the data of kachurovskiy](https://github.com/kachurovskiy/nanoels/) for the original NanoEls

# NanoEls H4 revised1 (currently unfinished work!!)
* This is an idea of an low cost variant with multiple placement and handsolder types for the components. For e.g.
    * PCB double sided 100mm x 100mm standard type
    * Design with free KiCad 8
    * All parts for hand solder (TCA8418 tricky, but possible - or usage of Adafruit Module)
    * One PCB for keypad and controls
    * Powering over USB or step-down converter with screw terminals
    * Interfacing to LCD via I2C interface over 4 pins possible (== same LCD type with I2C backpack PCB as sold on Aliexpress)
    * USB program and serial interface variants FT232RL or CH340 series
    * USB Connectors 90° types in two sides (idea: back connector>power, front connector>PC-control) or side entry both over 180° Types
    * ESP32-S3-WROOM-1 also possible (with antenna), currently used N16R8

* What's working in Hardware (with modified software)?
    * Display over I2C Interface
    * Programming and serial interface over FT232RL for the ESP32-S3-WROOM-1 (N16R8)
    * Keypad with direct soldered Keypad controller

* What's working in Software?
    * Software should be (i can not test it) compatible with the original NanoEls H4
    * Software working with ardunio Espressive ESP32 3.0.1 boards package (based on ESP-IDF v5.1.4). Maybe NOT backwards compatible to 2.x.
    * Display in I2C mode working. Faultless together with secondary I2C channel for keypad interface. 
    * Added LiquidCrystal_I2C_pt for handling of two I2C interfaces an changeable pins, because there are no available Arduino libraries with I2C pointer parameter.
    * Currently not completely tested, but encoder detects something. Hardware timer for ASYNC and A1 mode maybe not 100% working (unclear).

* ToDo's and current bugs
    * More testing of HW and SW
    * Integration in a real lathe
    * Making a low cost 3d-printable housing
    * PCB: Pitch of buzzer is false for the delivered type. Maybe a dual pitch package is sensible
    * PCB: Through hole pad for thermal connection of ESP32 module should have a bigger size free of solder mask
    * PCB: maybe dual package in standard sizes for buzzer free wheeling diode
    * PCB: upload the latest version

![PCB_H4revised1_3d_viewer](https://github.com/0unrouted0/nanoels/assets/165327246/6ad0018f-9c0e-4335-893f-ad6113fff6c5)
![PCB_H4revised1_pic1](https://github.com/0unrouted0/nanoels/assets/165327246/551250db-3250-49b3-9a3e-d763c4f59e51)
