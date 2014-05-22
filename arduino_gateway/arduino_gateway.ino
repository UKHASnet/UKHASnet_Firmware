/*

UKHASnet Gateway Code by Phil Crump M0DNY

Based on UKHASnet rf69_repeater by James Coxon M6JCX

*/

#include <SPI.h>
#include <string.h>
#include "RFM69Config.h"
#include "RFM69.h"
#include "NodeConfig.h"

//************* Misc Setup ****************/
uint8_t n;
uint32_t count = 1, data_interval = 2;
uint8_t data_count = 97; // 'a'
char data[64], string_end[] = "]";

// Singleton instance of the radio
RFM69 rf69(RFM_TEMP_FUDGE); // parameter: RFM temperature calibration offset (degrees as float)

#ifdef ENABLE_RFM_TEMPERATURE
int8_t sampleRfmTemp() {
    int8_t rfmTemp = rf69.readTemp();
    while(rfmTemp>100) {
        rfmTemp = rf69.readTemp();
    }
    rfmTemp-=RX_TEMP_FUDGE;
    return rfmTemp;
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
  
  return sprintf(data,"%s[%s]",data,id);
}

void setup() 
{
  analogReference(INTERNAL); // 1.1V ADC reference
  randomSeed(analogRead(6));
  Serial.begin(9600);
  delay(1000);
  
  while (!rf69.init()){
    delay(100);
  }
  
  int packet_len = gen_Data();
  rf69.send((uint8_t*)data, packet_len, rfm_power);
  
  rf69.setMode(RFM69_MODE_RX);
  rf69.SetLnaMode(RF_TESTLNA_SENSITIVE);
  
}

void loop()
{
  count++;
    
    for(int i=0;i<100;i++) {
      delay(10);
      
      if (rf69.checkRx()) {
        uint8_t buf[64];
        uint8_t len = sizeof(buf);
        int rx_rssi;
        
        rf69.recv(buf, &len);
        rx_rssi = rf69.lastRssi();
        Serial.print("rx: ");
        for (int i=0; i<len; i++) {
            if (buf[i] != ']') {
                Serial.print((char)buf[i]);
            } else {
                Serial.println((char)buf[i]);
                break;
            }
        }
        Serial.print("RSSI: ");
        Serial.println(rx_rssi);
      }
    }
  
  if (count >= data_interval){
    data_count++;

    if(data_count > 122){
      data_count = 98; //'b'
    }
    
    int packet_len = gen_Data();
    rf69.send((uint8_t*)data, packet_len, rfm_power);
    
    data_interval = random(BEACON_INTERVAL, BEACON_INTERVAL+20) + count;
    rf69.setMode(RFM69_MODE_RX);
    rf69.SetLnaMode(RF_TESTLNA_SENSITIVE);
  }
}
