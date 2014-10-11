arduino-repeater
==========

This is a repeater implementation designed to be used with an ATMega168/328 and the Arduino IDE.

This code implements the UKHASnet repeating protocol.

The 'zombie mode' is where the node will stop repeating, only transmitting it's own beacon packets, if the battery level is low. The threshold of this is configurable in NodeConfig.h

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

**SENSITIVE_RX**

This enables extra gain on the Receive LNA. Note: This has been found to cause receiver overload when other high-power (>50mW) nodes are within a few meters, these packets may therefore not be received.
