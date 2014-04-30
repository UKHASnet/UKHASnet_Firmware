#include "mbed.h"
#include "DS18B20.h"
#include "OneWireDefs.h"
#include "RFM69.h"

#define P4

// DEBUG_LEDS
// Blue Led - Receiving or Repeat TX
// Red Led - Own Beacon TX

//#define DEBUG_LEDS

#ifdef P2
char id[] = "P2"; // Callsign
int repeat_count=2;
#define REPEATER
char location_string[] = "24.4757,54.3449";
int min_beacon = 25;
int max_beacon = 35;
#define BATTV
#define DS_TEMP
float battery_fudge = 0.984; // Battery ADC calibration
float rfm_temp_fudge = 0;
uint8_t rfm_power = 17; // dBmW
#endif

#ifdef P3
char id[] = "P3"; // Callsign
int repeat_count=2;
#define REPEATER
char location_string[] = "24.4755,54.3449";
int min_beacon = 25;
int max_beacon = 35;
#define BATTV
#define DS_TEMP
float battery_fudge = 1.0; // Battery ADC calibration
float rfm_temp_fudge = 0;
uint8_t rfm_power = 17; // dBmW
#endif

#ifdef P4
char id[] = "P4"; // Callsign
int repeat_count=1;
//#define REPEATER
char location_string[] = "24.4756,54.3449";
int min_beacon = 100;
int max_beacon = 120;
float rfm_temp_fudge = 3.0;
uint8_t rfm_power = 17; // dBmW
#endif

#ifdef DEBUG_LEDS
DigitalOut redLed(LED_RED);
DigitalOut blueLed(LED_BLUE);
#endif

DS18B20 sensor1(true, true, false, PTB10);
float temp[4]; //used to store last 4 temperature values for averaging
float battv[4]; //used to store last 4 battery values for averaging
int zombie_status=0;

RFM69 rfm69(PTE4,PTE1,PTE3,PTE2,rfm_temp_fudge); // SPI2
DigitalOut rfm69_reset(PTE0);
Serial uart(PTC4, PTC3);  // UART debugging/gateway [tx, rx]

AnalogIn battV(PTB0);
AnalogIn randSeed(PTC2);

Timer beacon_delay;
int next_beacon = 0;
uint8_t data_count = 97; // 'a'

Timer sensor_poll;
int sensor_interval = 5;

Timer location_delay;
int next_location=0;
int location_interval = 1200; // seconds

char data_temp[RFM69_MAX_MESSAGE_LEN];

float getTemp();
float getBattv();
int getRand(int min,int max);

int construct_beacon() {
    if(data_count > 122){
        data_count = 98; //'b'
    }
    float extTemp = getTemp();
    // Start constructing the string
    sprintf(data_temp,"%d%c",repeat_count, data_count);
    if(location_delay.read()>next_location) {
        location_delay.stop();
        location_delay.reset();
        sprintf(data_temp,"%sL%s",data_temp,location_string);
        next_location = location_interval;
        location_delay.start();
    }
    sprintf(data_temp,"%sT%.1f",data_temp,extTemp);
    #ifdef BATTV
    float battery = getBattv();
    sprintf(data_temp,"%sV%.2fZ%d",data_temp,battery,zombie_status);
    #endif
    int n;
    n=sprintf(data_temp,"%s[%s]",data_temp,id);
    data_count++;
    return n;
}

int main() {
    #ifdef DEBUG_LEDS
    redLed=0;
    blueLed=1;
    #endif
    
    rfm69_reset = 1;
    wait_ms(100);
    rfm69_reset = 0;
    wait_ms(100);
    while (!rfm69.init()){
        rfm69_reset=0;
        wait(1);
        rfm69_reset=1;
        wait(1);
    }
    
    #ifdef DEBUG_LEDS
    redLed=1;
    #endif
    
    #ifdef DS_TEMP
    while (!sensor1.initialize()); // Setup DS18B20
    #endif
    
    // Set up beacon timer
    srand(randSeed*1000);
    next_beacon=0;
    next_location=0;
    beacon_delay.start();
    location_delay.start();
    for(int i=0;i<4;i++) {
        // Set up temperatue values for immediate beacon
        #ifdef DS_TEMP
        temp[i] = sensor1.readTemperature();
        #else
        temp[i] = rfm69.readTemp();
        while(temp[i]==255.0) {
            temp[i] = rfm69.readTemp();
        }
        #endif
        // Set up battery voltage values for immediate beacon
        #ifdef BATTV
        battv[i] = battV*3.3*2*battery_fudge;
        #endif
    }
    sensor_poll.start();
    
    // Main Loop
    while(1) {
        if (rfm69.mode()==RFM69_MODE_RX) {
            if (rfm69.checkRx()) { // Check for packet in RX buffer
                #ifdef DEBUG_LEDS
                blueLed=0;
                #endif
                uint8_t buf[64];
                uint8_t len = sizeof(buf);
                int rx_rssi;
  
                rfm69.recv(buf, &len);
                rx_rssi = rfm69.lastRssi();
  
                for (int i=0; i<len; i++) {
                    if (buf[i] == ']') {
                        //Print the string
                        buf[i+1] = '\0';
                        uart.printf("%s\r\n",(char*)buf);
                        uart.printf("RSSI: %d\r\n",rx_rssi);
                        #ifdef REPEATER
                        if (buf[0] > '0'){
                            //Reduce the repeat value
                            buf[0] = buf[0] - 1;
                            buf[i] = ',';
                            buf[i+1] = '\0';
              
                            strcpy(data_temp, (char*)buf); //first copy buf to data (bytes to char)
                            
                            if(strstr(data_temp, id) == 0){
                                int packet_len = sprintf(data_temp, "%s%s]", data_temp, id);
                                //random delay to try and avoid packet collision
                                wait_ms(getRand(100,800));
                                
                                uart.printf("%s\r\n",data_temp);
                                rfm69.send((uint8_t*)data_temp, packet_len, rfm_power);
                                break;
                            } else {
                                break;
                            }      
                        } else {
                            break;
                        }
                        #endif
                    }
                }
                #ifdef DEBUG_LEDS
                blueLed=1;
                #endif
            }
        }
            
        // Now check our own beacon interval
        //if(false) {
        if(beacon_delay.read()>next_beacon) {
            // Reset timer
            beacon_delay.stop();
            beacon_delay.reset();
            next_beacon=getRand(min_beacon,max_beacon);
            // Construct beacon
            int beacon_length = construct_beacon();
            // Send beacon packet
            #ifdef DEBUG_LEDS
            redLed= 0;
            #endif
            rfm69.send((uint8_t*)data_temp, beacon_length, rfm_power);
            #ifdef DEBUG_LEDS
            redLed= 1;
            #endif
            // Print to UART for debugging/gateway
            uart.printf("%s\r\n",data_temp);
            // Restart beacon timer
            beacon_delay.start();
        }
        
        // Sensor polling
        if(sensor_poll.read()>sensor_interval) {
            sensor_poll.stop();
            sensor_poll.reset();
            for(int i=0;i<3;i++) {
                temp[i] = temp[i+1];
                #ifdef BATTV
                battv[i] = battv[i+1];
                #endif
            }
            #ifdef DS_TEMP
            temp[3] = sensor1.readTemperature();
            #else
            temp[3] = rfm69.readTemp();
            while(temp[3]==255) {
                temp[3] = rfm69.readTemp();
            }
            #endif
            #ifdef BATTV
            battv[3] = battV*3.3*2*battery_fudge;
            #endif
            if (rfm69.mode()==RFM69_MODE_RX) {
                rfm69.sampleRssi();
                int floor_rssi = rfm69.lastRssi();
                uart.printf("Noise Floor: %d\r\n",floor_rssi);
            }
            sensor_poll.start();
        }
        
        #ifdef BATTV
        if(rfm69.mode()==RFM69_MODE_RX) {
            if(getBattv()<3.6) {
                rfm69.setMode(RFM69_MODE_SLEEP); // Turn off RX
                zombie_status=1;
            }
        } else if(rfm69.mode()==RFM69_MODE_SLEEP) {
            if(getBattv()>3.65) {
                rfm69.setMode(RFM69_MODE_RX); // Turn on RX
                zombie_status=0;
            }
        }
        #endif
    }
}

float getTemp() {
    float tempTemp=0.0;
    for(int i=0;i<4;i++) {
        tempTemp+=temp[i];
    }
    return tempTemp/4.0;
}

#ifdef BATTV
float getBattv() {
    float tempBattv=0.0;
    for(int i=0;i<4;i++) {
        tempBattv+=battv[i];
    }
    return tempBattv/4.0;
}
#endif

int getRand(int min,int max) {
    return min + (int)(rand()*(max-min+1.0)/(1.0+RAND_MAX));
}
