//************* Define Node that you're compiling for ****************/
#define JH

//************* Node-specific config ****************/
#ifdef JH
 char id[] = "JH";
 #define LOCATION_STRING "51.32091,-0.29663"
 uint8_t num_repeats = '1'; //The number of hops the message will make in the network
 #define BATTV_FUDGE 1.142 // Battery Voltage ADC Calibration
 #define BEACON_INTERVAL 10 // Beacon Interval in seconds
 uint8_t rfm_power = 10; // transmit power in dBm
 //#define DS18B20
 #define ENABLE_BATTV_SENSOR // Comment out to disable
 #define BATTV_PIN 0 //ADC 0 - Battery Voltage, scaled to 1.1V
 #define BATTV_SCALEFACTOR 5.7 // (R1+R2)/R2
#endif

