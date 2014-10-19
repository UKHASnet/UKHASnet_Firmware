UKHASnet Firmware
==========

This repository contains firmware for creating UKHASnet Sensor Nodes on embedded platforms such as AVR (arduino), KL25, etc.


To configure the nodes, copy 'NodeConfig-template.h' to 'NodeConfig.h' and fill in the settings for your nodes, this file will be included automatically at compilation.

arduino-repeater
======

This is a repeater implementation designed to be used with an ATMega168/328 and the Arduino IDE.

This code implements the UKHASnet repeating protocol.

The 'zombie mode' is where the node will stop repeating, only transmitting it's own beacon packets, if the battery level is low. If the battery level declines further, the node will stop transmitting altogether until the voltage recovers.

UART output, when enabled, allows the node to be used to submit packets generated, heard and repeated up to the UKHAS.net system.

arduino-sensor
======

This is a low-power sensor implementation designed to be used with an ATMega168/328,the Arduino IDE. Currently just Voltage and Temperature (through a DS18B20) is supported.

This does not repeat, instead it sleeps inbetween it's own beacons to save battery power. This can run for many months off a single set of cheap AA batteries (7 months so far and counting!).




All code in this repository is under the MIT License unless stated otherwise. Authors retain their copyright.
