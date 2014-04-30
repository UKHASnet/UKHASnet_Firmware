/*
UKHASnet rf69_repeater code

based on rf22_client.pde/ino from the RF22 library

*/

#include <SPI.h>
#include <string.h>
#include "RFM69Config.h"
#include "RFM69.h"
#include "LowPower.h"

//*************Node ID Setup ****************/
char id[] = "P5";
char location_string[] = "50.9387,-1.3979";
uint8_t rfm_power = 20; // dBmW

//*************Pin Setup ****************/
int batt_pin = 0; //ADC 0 - measure battery voltage

//*************Misc Setup ****************/
byte num_repeats = '3'; //The number of hops the message will make before stopping
float repeat_threshold = 3.65;
int zombie_mode = 1;
float slow_freq_threshold = 3.6;

float battV=0.0;
int n, count = 1, data_interval = 4;
byte data_count = 97; // 'a'
char data[64], string_end[] = "]";

// Singleton instance of the radio
RFM69 rf69(9.3);


void setupRFM69(){
  analogReference(INTERNAL); // 1.1V ADC reference
  while (!rf69.init()){
    delay(100);
  }
}

int gen_Data(){
  
  byte rfmTemp = rf69.readTemp();
  while(rfmTemp>100) {
    rfmTemp = rf69.readTemp();
  }
  
  battV = sampleBattv();
  char tempStr[14]; //make buffer large enough for 7 digits
  char* battStr;
  battStr = dtostrf(battV,7,2,tempStr);
  // Strip leading space
  while( (strlen(battStr) > 0) && (battStr[0] == 32) )
  {
     strcpy(battStr,&battStr[1]);
  }
  if(data_count=='a' or data_count=='z') {
    n=sprintf(data, "%c%cL%sT%dV%sZ%d[%s]", num_repeats, data_count, location_string, rfmTemp, battStr, zombie_mode, id);
  } else {
    n=sprintf(data, "%c%cT%dV%sZ%d[%s]", num_repeats, data_count, rfmTemp, battStr, zombie_mode, id);
  }
  return n;
}

void setup() 
{
  randomSeed(analogRead(6));
  delay(2000);
  
  while (!rf69.init()){
    delay(100);
  }
  delay(1000);
  
  int packet_len = gen_Data();
  rf69.send((uint8_t*)data, packet_len, rfm_power);
  delay(100);
}

void loop()
{
  count++;
  if(zombie_mode==0) {
    rf69.setMode(RFM69_MODE_RX);
    for(int i=0;i<32;i++) {
      LowPower.powerDown(SLEEP_250MS, ADC_OFF, BOD_OFF);
      if(rf69.checkRx()) {
        uint8_t buf[64];
        uint8_t len = sizeof(buf);
        rf69.recv(buf, &len);
        
        for (int i=0; i<len; i++) {
          if (buf[i] == ']') {
            buf[i+1] = '\0';
          }
        }
        
        // Need to take the recieved buffer and decode it and add a reference 
        if (buf[0] > '0') {
          //Reduce the repeat value
          buf[0] = buf[0] - 1;
          //Add the repeater ID
          //First find "]"
          for (int i=0; i<len; i++) {
            if (buf[i] == ']') {
              buf[i] = ',';
              buf[i+1] = '\0';
              
              strcpy(data, (char*)buf); //first copy buf to data (bytes to char)
  
              if(strstr(data, id) == 0) {
                int packet_len = sprintf(data, "%s%s]", data, id);
                
                packet_len = strlen(data);
                //random delay to try and avoid packet collision
                delay(random(50, 800));
                rf69.send((uint8_t*)data, packet_len, rfm_power);
                break;
              } else {
                break;
              }
            }
          }
        }
      }
    }
  } else {
    // Sleep for 8 seconds
    rf69.setMode(RFM69_MODE_SLEEP);
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
  
  if (count >= data_interval){  
    delay(100);
    data_count++;

    if(data_count > 122){
      data_count = 98; //'b'
    }
    
    int packet_len = gen_Data();
    rf69.send((uint8_t*)data, packet_len, rfm_power);
    
    if(battV > slow_freq_threshold) {
      data_interval = random(15, 20) + count;
      if(battV > repeat_threshold && zombie_mode==1) {
        rf69.setMode(RFM69_MODE_RX);
        zombie_mode=0;
      } else if (battV < repeat_threshold && zombie_mode==0) {
        rf69.setMode(RFM69_MODE_SLEEP);
        zombie_mode=1;
      }
    }
    else {
      data_interval = random(25, 30) + count;
    }
  }
}

float sampleBattv() {
  // External 4:1 Divider
  return ((float)analogRead(batt_pin)*1.1*4*2.24)/1023.0;
}
