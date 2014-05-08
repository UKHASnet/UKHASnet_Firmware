/*
UKHASnet rf69_repeater code

based on rf22_client.pde/ino from the RF22 library

*/

#include <SPI.h>
#include <string.h>
#include "RFM69Config.h"
#include "RFM69.h"
#include "LowPower.h"
#include "DallasTemperature.h"

#define P5

//************* Node ID Setup ****************/
#ifdef P5
char id[] = "P5";
char location_string[] = "50.93897,-1.39774";
#define BATTV_FUDGE 0.943
#endif
#define BEACON_INTERVAL 7
uint8_t rfm_power = 20; // dBmW

//************* Sensors ****************/
// Battery Voltage Measurement - Also enables zombie mode
#define ENABLE_BATTV_SENSOR // Comment out to disable, also disables zombie mode
#define BATTV_PIN 0 //ADC 0 - Battery Voltage, scaled to 1.1V
#define BATTV_FUDGE 1.109
DeviceAddress ds_addr;

//************* Misc Setup ****************/
byte num_repeats = '3'; //The number of hops the message will make before stopping
float battV=0.0;
uint8_t n;
uint32_t count = 1, data_interval = 10;
uint8_t data_count = 97; // 'a'
char data[64], string_end[] = "]";

// Singleton instance of the radio
RFM69 rf69(9.3); // parameter: RFM temperature calibration offset (degrees as float)
OneWire ow(9);  // on pin PB1 (arduino: 9)
DallasTemperature sensors(&ow);

#ifdef ENABLE_BATTV_SENSOR
float sampleBattv() {
  // External 5:1 Divider
  return ((float)analogRead(BATTV_PIN)*1.1*5*BATTV_FUDGE)/1023.0;
}
#endif

int gen_Data(){
  if(data_count=='a' or data_count=='z') {
      sprintf(data, "%c%cL%s", num_repeats, data_count, location_string);
  } else {
      sprintf(data, "%c%c", num_repeats, data_count);
  }
  
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();
  LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
  
  float temp = sensors.getTempC(ds_addr);

  char* tempStr;
  char tempStrA[12]; //make buffer large enough for 7 digits
  tempStr = dtostrf(temp,6,1,tempStrA);
  while( (strlen(tempStr) > 0) && (tempStr[0] == 32) )
  {
     strcpy(tempStr,&tempStr[1]);
  }
  sprintf(data,"%sT%s",data,tempStr);
  
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
  
  return sprintf(data,"%s[%s]",data,id);
}

void setup() 
{
  analogReference(INTERNAL); // 1.1V ADC reference
  randomSeed(analogRead(6));
  delay(1000);
  
  sensors.begin();
  sensors.getAddress(ds_addr, 0);
  sensors.setResolution(ds_addr, 12);
  
  while (!rf69.init()){
    delay(100);
  }
  
  int packet_len = gen_Data();
  rf69.send((uint8_t*)data, packet_len, rfm_power);
}

void loop()
{
  count++;
  
  rf69.setMode(RFM69_MODE_SLEEP);
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);

  if (count >= data_interval){
    data_count++;

    if(data_count > 122){
      data_count = 98; //'b'
    }
    
    int packet_len = gen_Data();
    rf69.send((uint8_t*)data, packet_len, rfm_power);
    
    data_interval = random(BEACON_INTERVAL, BEACON_INTERVAL+10) + count;
  }
}
