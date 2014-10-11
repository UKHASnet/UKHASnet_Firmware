//************* Define Node that you're compiling for ****************/
#define ZZ00

//************* Node-specific config ****************/
#ifdef XX00
 char id[] = "XX00";
 #define LOCATION_STRING "50.3475,-1.33445"
 byte num_repeats = '1'; //The number of hops the message will make in the network
 #define BATTV_FUDGE 1.109 // Battery Voltage ADC Calibration
 #define BEACON_INTERVAL 120 // Beacon Interval in seconds
 uint8_t rfm_power = 20; // dBmW
 #define DS18B20
 #define ENABLE_BATTV_SENSOR // Comment out to disable
 #define BATTV_PIN 0 //ADC 0 - Battery Voltage, scaled to 1.1V
#endif

#ifdef YY01
 char id[] = "YY01";
 #define LOCATION_STRING "50.9346,-1.4531"
 byte num_repeats = '1'; //The number of hops the message will make in the network
 #define BATTV_FUDGE 1.109 // Battery Voltage ADC Calibration
 #define BEACON_INTERVAL 120 // Beacon Interval in seconds
 uint8_t rfm_power = 10; // dBmW
 #define DHT22
 #define ENABLE_BATTV_SENSOR // Comment out to disable
 #define BATTV_PIN 0 //ADC 0 - Battery Voltage, scaled to 1.1V
#endif

//************* Other config ****************/
#define RFM_TEMP_FUDGE 0
