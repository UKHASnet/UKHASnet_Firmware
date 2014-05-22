//************* Define Node that you're compiling for ****************/
#define P02

//************* Node-specific config ****************/
#ifdef P01
char id[] = "P01";
char location_string[] = "50.93889,-1.39782";
byte num_repeats = '3'; //The number of hops the message will make in the network
#define BATTV_FUDGE 0.935 // Battery Voltage ADC Calibration
#define BEACON_INTERVAL 55 // Beacon Interval is (x * 8) seconds
uint8_t rfm_power = 20; // dBmW
#define SENSITIVE_RX // Enables TESTLNA_SENSITIVE
#endif
#ifdef P02
char id[] = "P02";
char location_string[] = "50.93805,-1.39729";
byte num_repeats = '3'; //The number of hops the message will make in the network
#define BATTV_FUDGE 0.943 // Battery Voltage ADC Calibration
#define BEACON_INTERVAL 50 // Beacon Interval is (x * 8) seconds
uint8_t rfm_power = 20; // dBmW
#define SENSITIVE_RX // Enables TESTLNA_SENSITIVE
#endif

//************* Other config ****************/
// Battery Voltage Measurement - Also enables zombie mode
#define ENABLE_BATTV_SENSOR // Comment out to disable, also disables zombie mode
#define BATTV_PIN 0 //ADC 0 - Battery Voltage, scaled to 1.1V
// RFM Temperature Sensor - Not very accurate and sometimes glitchy
#define ENABLE_RFM_TEMPERATURE // Comment out to disable
#define RFM_TEMP_FUDGE 0 // Initial RFM Calibration
#define RX_TEMP_FUDGE 5.0 // Temperature offset when in RX due to self-heating

//************* Power Saving ****************/
#ifdef ENABLE_BATTV_SENSOR
  #define ENABLE_ZOMBIE_MODE // Comment this out to disable
#endif
#define ZOMBIE_THRESHOLD 3.65
