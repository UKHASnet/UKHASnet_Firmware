arduino-sensor
==========

This is a low-power sensor implementation designed to be used with an ATMega168/328 and the Arduino IDE.

Sensors Supported:
* DS18B20 (Temperature)
* DHT22 (Temperature + Humidity)

This does not repeat, instead it sleeps inbetween it's own beacons to save battery power. This can run for many months off a single set of cheap AA batteries (7 months so far and counting!).


NodeConfig.h
======

Copy NodeConfig-template.h to NodeConfig.h to set up code for your nodes.

**char id[]**

This sets the ID, or callsign, of the repeater. This should be unique within the network, but as short as possible. You can see existing IDs on ukhas.net

**LOCATION_STRING**

If this is set then it will be sent as a location at boot and then every 25 packets. This sets the Lat/Long of the node on the map at ukhas.net

If you don't wish your location to be visible, comment out this line to disable it.

**BEACON_INTERVAL**

The time interval between the nodes own beacons will be a random number of seconds between (this value)x8 and (this value+10)x8

**rfm_power**

This configures the Transmit Power of the RFM69, in dBmW. eg. 1dBmW = 1mW, 10dBmW = 10mW, 17dBmW = 50mW, 20dBm = 100mW.

Note: Only the RFM69*H*W can transmit at more than 50mW.

**SENSOR_VCC**

This should be enabled when the sensor (DS18B20/DHT22) uses pin 'PB0' (Arduino 'D8') for positive voltage supply. This will allow the sensor to be fully powered down while the microcontroller is in sleep. The sensor will be powered up 15ms before communication is attempted.
