//************* Define Node that you're compiling for ****************/
#define PB1

//************* Node-specific config ****************/
#ifdef PB1
char id[] = "PB1";
byte num_repeats = '2'; //The number of hops the message will make in the network
#define BATTV_FUDGE 1.098
#define BEACON_INTERVAL 20
uint8_t rfm_power = 10; // dBmW
#endif

//************* Other config ****************/
// RFM Temperature Sensor - Not very accurate and sometimes glitchy
#define ENABLE_RFM_TEMPERATURE // Comment out to disable
#define RFM_TEMP_FUDGE 0 // Initial RFM Calibration
#define RX_TEMP_FUDGE 5.0 // Temperature offset when in RX due to self-heating
// Battery Voltage Measurement
#define ENABLE_BATTV_SENSOR // Comment out to disable
#define BATTV_PIN 0 //ADC 0 - Battery Voltage, scaled to 1.1V
