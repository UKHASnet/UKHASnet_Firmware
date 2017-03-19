/************* Define Node that you're compiling for ****************/
/* This allows you to store multiple node configs in this file */ 
#define CHANGEME

//************* Node-specific config ****************/
#ifdef CHANGEME
    /** The string identifier of the node */
    #define NODE_ID                 "CHANGEME"

    /** The location of this node in decimal degrees. Comment out to disable
     * transmission of the location of this node */
    #define LOCATION_STRING         "51.10000,-0.10000"

    /** The message will make this many hops in the network. */
    #define NUM_REPEATS             1

    /** The microcontroller ADC and/or the potential divider can be inaccurate.
     * Calibrate them using this value */
    #define BATTV_FUDGE             1.0

    /** The node will transmit a packet with this interval (in seconds) */
    #define BEACON_INTERVAL         30

    /** Set the RFM69 radio to this power level (given in dBm). Note that non
     * -HW models support only up to +13dBm */
    #define RFM_POWER               10

    /** Set this to 1 to enable reporting of the battery voltage using the AVR
     * ADC and an external potential divider. Remember to set the divider ratio
     * BATTV_SCALEFACTOR and the ADC pin BATTV_PIN */
    #define ENABLE_BATTV_SENSOR     1

    /** The ADC pin (e.g. 0 for ADC0) on which the battery monitoring potential
     * divider is placed.
     * @warning This is NOT the general pin number on the device
     */
    #define BATTV_PIN               0

    /** The scale factor for the potential divider for measuring battery
     * voltage. This is calculated as (R1+R2)/R2 where R1 is the 'top' divider
     * in the chain the R2 the bottom.
     *
     * The correct value for the M0DNY AVR Sensor V3 PCB is: 5.7
     */
    #define BATTV_SCALEFACTOR       5.7 // (R1+R2)/R2
#endif

