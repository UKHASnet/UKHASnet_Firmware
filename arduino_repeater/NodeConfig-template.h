//************* Define Node that you're compiling for ****************/
#define YYYY

//************* Node-specific config ****************/
#ifdef YYYY
 char id[] = "YYYY";
 #define LOCATION_STRING "50.4535,-1.35435"
 byte num_repeats = '3'; //The number of hops the message will make in the network
 #define BEACON_INTERVAL 300 // Beacon Interval in seconds
 uint8_t rfm_power = 20; // dBmW
 #define SENSITIVE_RX // Enables TESTLNA_SENSITIVE
 #define ENABLE_BATTV_SENSOR // Comment out to disable, you must also disable zombie mode
 #define BATTV_PIN 0 //ADC 0 - Battery Voltage, scaled to 1.1V
 #define BATTV_FUDGE 0.935 // Battery Voltage ADC Calibration Factor
 // Power Saving
 #define ENABLE_ZOMBIE_MODE // Comment this out to disable
 #define ZOMBIE_THRESHOLD 3.65 // Lower Voltage Threshold
 #define ENABLE_UART_OUTPUT // UART output of packets and rssi, used for gateways
#endif

#ifdef XXXX
 char id[] = "XXXX";
 #define LOCATION_STRING "50.2475,-1.36545"
 byte num_repeats = '3'; //The number of hops the message will make in the network
 #define BEACON_INTERVAL 300 // Beacon Interval in seconds
 uint8_t rfm_power = 20; // dBmW
 #define SENSITIVE_RX // Enables TESTLNA_SENSITIVE
 #define ENABLE_BATTV_SENSOR // Comment out to disable, you must also disable zombie mode
 #define BATTV_PIN 0 //ADC 0 - Battery Voltage, scaled to 1.1V
 #define BATTV_FUDGE 0.943 // Battery Voltage ADC Calibration Factor
 // Power Saving
 #define ENABLE_ZOMBIE_MODE // Comment this out to disable
 #define ZOMBIE_THRESHOLD 3.65 // Lower Voltage Threshold
#endif

//************* Other config ****************/
// RFM Temperature Sensor - Not very accurate and sometimes glitchy
#define ENABLE_RFM_TEMPERATURE // Comment out to disable
#define RFM_TEMP_FUDGE 0 // Initial RFM Calibration
#define RX_TEMP_FUDGE 5.0 // Temperature offset when in RX due to self-heating
