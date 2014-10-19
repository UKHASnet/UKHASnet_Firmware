/*

UKHASnet Repeater Code by Phil Crump M0DNY

Based on UKHASnet rf69_repeater by James Coxon M6JCX

*/

#include <SPI.h>
#include <string.h>
#include "RFM69Config.h"
#include "RFM69.h"
#include "LowPower.h"
#include "NodeConfig.h"

//************* Misc Setup ****************/
float battV=0.0;
uint8_t n, j;
uint32_t count = 1, data_interval = 2; // Initially send a couple of beacons in quick succession
uint8_t zombie_mode; // Stores current status: 0 - Full Repeating, 1 - Low Power shutdown, (beacon only)
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

  #ifdef LOCATION_STRING
   if(data_count=='a' or data_count=='z') {
       sprintf(data, "%c%cL%s", num_repeats, data_count, LOCATION_STRING);
   } else {
       sprintf(data, "%c%c", num_repeats, data_count);
   }
  #else
   sprintf(data, "%c%c", num_repeats, data_count);
  #endif
  
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
     #ifdef SENSITIVE_RX
      rf69.SetLnaMode(RF_TESTLNA_SENSITIVE);
     #endif
   } else {
     rf69.setMode(RFM69_MODE_SLEEP);
     zombie_mode=1;
   }
  #else
   rf69.setMode(RFM69_MODE_RX);
   zombie_mode=0;
   #ifdef SENSITIVE_RX
    rf69.SetLnaMode(RF_TESTLNA_SENSITIVE);
   #endif
  #endif
  
  #ifdef ENABLE_UART_OUTPUT
   // Print out own beacon packet
   for (j=0; j<packet_len; j++)
   {
     if(data[j]==']')
     {
       Serial.println(data[j]);
       break;
     }
     else
     {
       Serial.print(data[j]);
     }
   }
  #endif
}

void loop()
{
  count++;
  
  if(zombie_mode==0) {
    rf69.setMode(RFM69_MODE_RX);
    
    for(uint8_t i=0;i<255;i++) {
      LowPower.powerDown(SLEEP_30MS, ADC_OFF, BOD_OFF);
      
      if (rf69.checkRx()) {
        uint8_t buf[64];
        uint8_t len = sizeof(buf);
        int rx_rssi;
        
        rf69.recv(buf, &len);
        
        #ifdef ENABLE_UART_OUTPUT
         rx_rssi = rf69.lastRssi();
         for (j=0; j<len; j++) {
             Serial.print((char)buf[j]);
             if(buf[j]==']') break;
         }
         Serial.print("|");
         Serial.println(rx_rssi);
        #endif

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
          int packet_len = end_bracket + sprintf((char *)&buf[end_bracket], ",%s]", id);

          //random delay to try and avoid packet collision
          delay(random(50, 800));
          
          rf69.send((uint8_t*)buf, packet_len, rfm_power);
          #ifdef ENABLE_UART_OUTPUT
           // Print repeated packet
           for (j=0; j<packet_len; j++)
           {
             if(data[j]==']')
             {
               Serial.println(data[j]);
               break;
             }
             else
             {
               Serial.print(data[j]);
             }
           }
          #endif
        }
      }
    }
  } else {
    // Battery Voltage Low - Zombie Mode
    
    // Low Power Sleep for 8 seconds
    rf69.setMode(RFM69_MODE_SLEEP);
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
  
  if (count >= data_interval){
    data_count++;

    if(data_count > 122){
      data_count = 98; //'b'
    }
    
    int packet_len = gen_Data();
    rf69.send((uint8_t*)data, packet_len, rfm_power);
    #ifdef ENABLE_UART_OUTPUT
       // Print out own beacon packet
       for (j=0; j<packet_len; j++)
       {
         if(data[j]==']')
         {
           Serial.println(data[j]);
           break;
         }
         else
         {
           Serial.print(data[j]);
         }
       }
    #endif
    
    data_interval = random((BEACON_INTERVAL/8), (BEACON_INTERVAL/8)+2) + count;
    #ifdef ENABLE_ZOMBIE_MODE
     if(battV > ZOMBIE_THRESHOLD && zombie_mode==1) {
         rf69.setMode(RFM69_MODE_RX);
         zombie_mode=0;
         #ifdef SENSITIVE_RX
          rf69.SetLnaMode(RF_TESTLNA_SENSITIVE);
         #endif
     } else if (battV < ZOMBIE_THRESHOLD && zombie_mode==0) {
         rf69.setMode(RFM69_MODE_SLEEP);
         zombie_mode=1;
     }
    #endif
  }
}
