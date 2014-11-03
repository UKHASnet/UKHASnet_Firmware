ChangeLog
==========


3rd November 2014
======

**New RFM lib revision**

'TEMP_FUDGE' has been stripped out. Un-necessary delays have been removed. The init() function now checks for a non-zero result on the RFM firmware version register, and will return false on a zero result - indicating that a device is likely not connected/not functioning.

**Code Optimisations**

Most variables are declared globally to reduce complexity. This turned up one instance of a loop-counting variable being re-used inside the loop. This has been corrected.

**UART Output Fixed**

The UART is now not slept when waiting for packets in non-zombie mode. This fixes corruption of the UART caused by the peripheral being shutdown/booted up 30 times per second.
