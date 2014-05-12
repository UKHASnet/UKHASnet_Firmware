/*

UKHASnet Repeater Code by Phil Crump M0DNY

Based on UKHASnet rf69_repeater by James Coxon M6JCX

*/

#include <SPI.h>
#include <string.h>
#include "RFM69Config.h"
#include "RFM69.h"
#include "LowPower.h"

#define P5

//************* Node ID Setup ****************/
#ifdef P01
char id[] = "P01";
char location_string[] = "50.93753,-1.39797";
#define BATTV_FUDGE 0.935
#endif
#ifdef P02
char id[] = "P02";
char location_string[] = "50.93623,-1.39876";
#define BATTV_FUDGE 0.943
#endif
#ifdef P5
char id[] = "P5";
char location_string[] = "50.93897,-1.39774";
#define BATTV_FUDGE 0.943
#endif
#define BEACON_INTERVAL 30
uint8_t rfm_power = 20; // dBmW

//************* Sensors ****************/
// Battery Voltage Measurement - Also enables zombie mode
#define ENABLE_BATTV_SENSOR // Comment out to disable, also disables zombie mode
#define BATTV_PIN 0 //ADC 0 - Battery Voltage, scaled to 1.1V
#define BATTV_FUDGE 0.943
// RFM Temperature Sensor - Not very accurate and sometimes glitchy
#define ENABLE_RFM_TEMPERATURE // Comment out to disable
#define RX_TEMP_FUDGE 5.0 // Temperature offset when in RX due to self-heating

//************* Power Saving ****************/
#ifdef ENABLE_BATTV_SENSOR
  #define ENABLE_ZOMBIE_MODE // Comment this out to disable
#endif
uint8_t zombie_mode; // Stores current status: 0 - Full Repeating, 1 - Low Power shutdown, (beacon only)
#define ZOMBIE_THRESHOLD 3.65

//************* Misc Setup ****************/
byte num_repeats = '3'; //The number of hops the message will make before stopping
float battV=0.0;
uint8_t n;
uint32_t count = 1, data_interval = 10;
uint8_t data_count = 97; // 'a'
char data[64], string_end[] = "]";

// Singleton instance of the radio
RFM69 rf69(9.3); // parameter: RFM temperature calibration offset (degrees as float)

#ifdef ENABLE_RFM_TEMPERATURE
int8_t sampleRfmTemp() {
    int8_t rfmTemp = rf69.readTemp();
    while(rfmTemp>100) {
        rfmTemp = rf69.readTemp();
    }
    if(zombie_mode==0) {
        rfmTemp-=RX_TEMP_FUDGE;
    }
    return rfmTemp;
}
#endif

#ifdef ENABLE_BATTV_SENSOR
float sampleBattv() {
  // External 4:1 Divider
  return ((float)analogRead(BATTV_PIN)*1.1*4*BATTV_FUDGE)/1023.0;
}
#endif

int gen_Data(){
  if(data_count=='a' or data_count=='z') {
      sprintf(data, "%c%cL%s", num_repeats, data_count, location_string);
  } else {
      sprintf(data, "%c%c", num_repeats, data_count);
  }
  
  #ifdef ENABLE_RFM_TEMPERATURE
  sprintf(data,"%sT%d",data,sampleRfmTemp());
  #endif
  
  #ifdef ENABLE_BATTV_SENSOR
  battV = sampleBattv();
  char* battStr;
  char tempStrB[14]; //make buffer large enough for 7 digits
  battStr = dtostrf(battV,7,2,tempStrB);
  while( (strlen(battStr) > 0) && (battStr[0] == 32) )
  {
     strcpy(battStr,&battStr[1]);
  }
  sprintf(data,"%sV%s",data,battStr);
  #endif
  
  #ifdef ENABLE_ZOMBIE_MODE
  sprintf(data,"%sZ%d",data,zombie_mode);
  #endif
  
  return sprintf(data,"%s[%s]",data,id);
}

void setup() 
{
  analogReference(INTERNAL); // 1.1V ADC reference
  randomSeed(analogRead(6));
  delay(1000);
  
  while (!rf69.init()){
    delay(100);
  }
  
  int packet_len = gen_Data();
  rf69.send((uint8_t*)data, packet_len, rfm_power);
  
  #ifdef ENABLE_ZOMBIE_MODE
  if(battV > ZOMBIE_THRESHOLD) {
    rf69.setMode(RFM69_MODE_RX);
    zombie_mode=0;
  } else {
    rf69.setMode(RFM69_MODE_SLEEP);
    zombie_mode=1;
  }
  #else
  rf69.setMode(RFM69_MODE_RX);
  zombie_mode=0;
  #endif
}

void loop()
{
  count++;
  
  if(zombie_mode==0) {
    rf69.setMode(RFM69_MODE_RX);
    
    for(int i=0;i<64;i++) {
      LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF);
      
      if (rf69.checkRx()) {
        uint8_t buf[64];
        uint8_t len = sizeof(buf);
        
        rf69.recv(buf, &len);

        // find end of packet & start of repeaters
        int end_bracket = -1, start_bracket = -1;        
        for (int i=0; i<len; i++) {
          if (buf[i] == '[') {
            start_bracket = i;
          }
          else if (buf[i] == ']') {
            end_bracket = i;
            buf[i+1] = '\0';
            break;
          }
        }

        // Need to take the recieved buffer and decode it and add a reference 
        if (buf[0] > '0' && end_bracket != -1 && strstr((const char *)&buf[start_bracket], id) == NULL) {
          // Reduce the repeat value
          buf[0]--;
          
          // Add the repeater ID
          int packet_len = end_bracket + sprintf((char *)&buf[end_bracket], ",%s]", (const char *)id);

          //random delay to try and avoid packet collision
          delay(random(50, 800));
          
          rf69.send((uint8_t*)data, packet_len, rfm_power);
        }
      }
    }
  } else {
    // Sample Sensors here..
    
    // Sleep for 8 seconds
    rf69.setMode(RFM69_MODE_SLEEP);
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
  
  if (count >= data_interval){
    count = 0;

    data_count++;
    
    if(data_count > 122){
      data_count = 98; //'b'
    }
    
    int packet_len = gen_Data();
    rf69.send((uint8_t*)data, packet_len, rfm_power);
    
    data_interval = random(BEACON_INTERVAL, BEACON_INTERVAL+10) + count;
    #ifdef ENABLE_ZOMBIE_MODE
    if(battV > ZOMBIE_THRESHOLD && zombie_mode==1) {
        rf69.setMode(RFM69_MODE_RX);
        zombie_mode=0;
    } else if (battV < ZOMBIE_THRESHOLD && zombie_mode==0) {
        rf69.setMode(RFM69_MODE_SLEEP);
        zombie_mode=1;
    }
    #endif
  }
}
