/*

UKHASnet Dumb-Sensor Code by Phil Crump M0DNY

Based on UKHASnet rf69_repeater by James Coxon M6JCX

*/

#include <SPI.h>
#include <string.h>

#include <UKHASnetRFM69.h>
#include <UKHASnetRFM69-config.h>

#include "LowPower.h"

#include "NodeConfig.h"

#ifdef DS18B20
 #include "DallasTemperature.h"
 DeviceAddress ds_addr;
 OneWire ow(9);  // on pin PB1 (arduino: 9)
 DallasTemperature sensors(&ow);
#endif

#ifdef DHT22
 #include "dht.h"
 dht DHT;
 #define DHT22_PIN 9 // Arduino pin 9
#endif

//************* Misc Setup ****************/
float battV=0.0;
uint8_t n;
uint32_t count = 1, data_interval = 2;
uint8_t data_count = 97; // 'a'
char data[64], string_end[] = "]";

#ifdef ENABLE_BATTV_SENSOR
 float sampleBattv() {
   // External 5:1 Divider
   return ((float)analogRead(BATTV_PIN)*1.1*5*BATTV_FUDGE)/1023.0;
 }
#endif

int gen_Data(){

  #ifdef LOCATION_STRING
   if(data_count=='a' or data_count=='z') {
       sprintf(data, "%c%cL%s", num_repeats, data_count, LOCATION_STRING);
   } else {
       sprintf(data, "%c%c", num_repeats, data_count);
   }
  #else
   sprintf(data, "%c%c", num_repeats, data_count);
  #endif
  
  #ifdef SENSOR_VCC
   digitalWrite(8, HIGH);
   LowPower.powerDown(SLEEP_15MS, ADC_OFF, BOD_OFF);
  #endif
  
  // Temperature (DS18B20 or DHT22)
  
  #ifdef DS18B20
   sensors.setWaitForConversion(false);
   sensors.requestTemperatures();
   LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
  
   float temp = sensors.getTempC(ds_addr);
  #endif
  
  #ifdef DHT22
   int chk = DHT.read22(DHT22_PIN);

   while(chk!=DHTLIB_OK)
   {
     LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
     chk = DHT.read22(DHT22_PIN);
   }
   float temp = DHT.temperature;
  #endif
  
  char* tempStr;
  char tempStrA[12]; //make buffer large enough for 7 digits
  tempStr = dtostrf(temp,6,1,tempStrA);
  while( (strlen(tempStr) > 0) && (tempStr[0] == 32) )
  {
     strcpy(tempStr,&tempStr[1]);
  }
  sprintf(data,"%sT%s",data,tempStr);
  
  // Humidity (DHT22)
  
  #ifdef DHT22
   float humid = DHT.humidity;
  
   tempStr = dtostrf(humid,6,1,tempStrA);
   while( (strlen(tempStr) > 0) && (tempStr[0] == 32) )
   {
      strcpy(tempStr,&tempStr[1]);
   }
   sprintf(data,"%sH%s",data,tempStr);
  #endif
  
  #ifdef SENSOR_VCC
   digitalWrite(8, LOW);
  #endif
  
  // Battery Voltage
  
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
  
  #ifdef SENSOR_VCC
   pinMode(8, OUTPUT);
   digitalWrite(8, LOW);
  #endif
  
  while(rf69_init() != RFM_OK)
    LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF);
  
  #ifdef DS18B20
   #ifdef SENSOR_VCC
    digitalWrite(8, HIGH);
    LowPower.powerDown(SLEEP_15MS, ADC_OFF, BOD_OFF);
   #endif
   sensors.begin();
   sensors.getAddress(ds_addr, 0);
   sensors.setResolution(ds_addr, 12);
   #ifdef SENSOR_VCC
    digitalWrite(8, LOW);
   #endif
  #endif
  
  int packet_len = gen_Data();
  rf69_send((rfm_reg_t *)data, packet_len, RFM_POWER);
}

void loop()
{
  count++;
  
  rf69_set_mode(RFM69_MODE_SLEEP);
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);

  if (count >= data_interval){
    data_count++;

    if(data_count > 122){
      data_count = 98; //'b'
    }
    
    int packet_len = gen_Data();
    rf69_send((rfm_reg_t *)data, packet_len, RFM_POWER);
    
    data_interval = random((BEACON_INTERVAL/8), (BEACON_INTERVAL/8)+2) + count;
  }
}
