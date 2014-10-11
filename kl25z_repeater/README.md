UKHASnet-frdmKL25
============

**This port is not actively maintained, is well behind current development and suspected to include bugs.**

This is a UKHASnet node implementation for the Freescale KL25z Freedom Boards, using the mbed libraries and online compiler.

Individual nodes are configured in the '#ifdef's at the top, so feel free to remove mine and add your own. The idea is that then you can rebuild for each of your nodes by just changing the '#define P4' line.

## Libraries

### OneWireCRC

* Used for DS18B20 Temperature Sensors

### RFM69

* UKHASnet Synchronous RFM69 868MHz Transceiver Lib
