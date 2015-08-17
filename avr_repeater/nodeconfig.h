//************* Define Node that you're compiling for ****************/
#define JH

//************* Node-specific config ****************/
#ifdef JH
    /** The string identifier of the node */
    #define NODE_ID                 "SM2"

    /** The location of this node in decimal degrees. Comment out to disable
     * transmission of the location of this node */
    /*#define LOCATION_STRING         "51.10000,-0.10000"*/

    /** The message will make this many hops in the network. */
    #define NUM_REPEATS             0

    /** The microcontroller ADC and/or the potential divider are inaccurate.
     * Calibrate them using this value */
    #define BATTV_FUDGE             1.142

    /** The node will transmit a packet with this interval (in seconds) */
    #define BEACON_INTERVAL         10

    /** Set the RFM69 radio to this power level (given in dBm). Note that non
     * -HW models support only up to +13dBm */
    #define RFM_POWER               10

    /** Set this line to 1 if there is a DS18B20 temperature sensor connected
     * to the node and you would like the firmware to read from it and transmit
     * the value */
    #define DS18B20                 1

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
     */
    #define BATTV_SCALEFACTOR       5.7 // (R1+R2)/R2
#endif

