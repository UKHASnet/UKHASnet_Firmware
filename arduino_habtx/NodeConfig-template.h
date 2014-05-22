//************* Define Node that you're compiling for ****************/
#define PS00

//************* Node-specific config ****************/
#ifdef PS00
char id[] = "PS00";
char location_string[] = "50.93889,-1.39782";
byte num_repeats = '1'; //The number of hops the message will make in the network
#define BATTV_FUDGE 1.109 // Battery Voltage ADC Calibration
#define BEACON_INTERVAL 15 // Beacon Interval is (x * 8) seconds
uint8_t rfm_power = 20; // dBmW
#endif

#ifdef PS01
char id[] = "PS01";
char location_string[] = "50.93805,-1.39729";
byte num_repeats = '1'; //The number of hops the message will make in the network
#define BATTV_FUDGE 1.109 // Battery Voltage ADC Calibration
#define BEACON_INTERVAL 15 // Beacon Interval is (x * 8) seconds
uint8_t rfm_power = 10; // dBmW
#endif

#ifdef PS02
char id[] = "PS02";
char location_string[] = "50.93877,-1.3979";
byte num_repeats = '1'; //The number of hops the message will make in the network
#define BATTV_FUDGE 1.109 // Battery Voltage ADC Calibration
#define BEACON_INTERVAL 15 // Beacon Interval is (x * 8) seconds
uint8_t rfm_power = 10; // dBmW
#endif

//************* Other config ****************/
#define ENABLE_BATTV_SENSOR // Comment out to disable
#define BATTV_PIN 0 //ADC 0 - Battery Voltage, scaled to 1.1V
#define RFM_TEMP_FUDGE 0
