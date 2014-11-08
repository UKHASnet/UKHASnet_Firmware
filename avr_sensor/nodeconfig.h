//************* Define Node that you're compiling for ****************/
#define J1

//************* Node-specific config ****************/
#ifdef J1
 char id[] = "J1";
 #define LOCATION_STRING "51.32091,-0.29663"
 byte num_repeats = '1'; //The number of hops the message will make in the network
 #define BATTV_FUDGE 1.109 // Battery Voltage ADC Calibration
 #define BEACON_INTERVAL 120 // Beacon Interval in seconds
 uint8_t rfm_power = 10; // dBm
 //#define DS18B20
 //#define ENABLE_BATTV_SENSOR // Comment out to disable
 //#define BATTV_PIN 0 //ADC 0 - Battery Voltage, scaled to 1.1V
#endif

