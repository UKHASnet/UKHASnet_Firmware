/*

UKHASnet Dumb-Buoy Code by Phil Crump M0DNY

Based on UKHASnet rf69_repeater by James Coxon M6JCX

GPS Code from Habduino by Anthony Stirk M0UPU - https://github.com/HABduino/HABduino/tree/master/Software/
* Based on CUSF Joey Flight Code - https://github.com/cuspaceflight/joey-m/tree/master/firmware

*/
#include <util/crc16.h>
#include <SPI.h>
#include <string.h>
#include "RFM69Config.h"
#include "RFM69.h"
#include "LowPower.h"
#include "DallasTemperature.h"
#include "NodeConfig.h"


//************* Sensors ****************/
// Battery Voltage Measurement - Also enables zombie mode
#define ENABLE_BATTV_SENSOR // Comment out to disable, also disables zombie mode
#define BATTV_PIN 0 //ADC 0 - Battery Voltage, scaled to 1.1V

//************* Misc Setup ****************/
DeviceAddress ds_addr;
float battV=0.0;
uint8_t n;
uint32_t count = 1, data_interval = 2;
uint8_t data_count = 97; // 'a'
char data[64], string_end[] = "]";

// Singleton instance of the radio
RFM69 rf69(RFM_TEMP_FUDGE); // parameter: RFM temperature calibration offset (degrees as float)
OneWire ow(9);  // on pin PB1 (arduino: 9)
DallasTemperature sensors(&ow);

uint8_t buf[60]; 
uint8_t lock =0, sats = 0, nolock=0, psm_status = 0;
int16_t GPSerror = 0,lat_int=0,lon_int=0;
int32_t lat = 0, lon = 0, alt = 0, lat_dec = 0, lon_dec =0;

#ifdef ENABLE_BATTV_SENSOR
float sampleBattv() {
  // External 5:1 Divider
  return ((float)analogRead(BATTV_PIN)*1.1*5*BATTV_FUDGE)/1023.0;
}
#endif

int gen_Data(){
  sprintf(data, "%c%c", num_repeats, data_count);
  
  gps_check_lock();
  if(lock<2) {
    if(psm_status==1) {
        setGps_MaxPerformanceMode();
        psm_status=0;
    }
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    gps_check_lock();
    if(lock<2) {
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
        gps_check_lock();
        nolock++;
        if(lock<2 && nolock>10) {
            resetGPS();
            LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF);
            setupGPS();
            LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF);
            setGps_MaxPerformanceMode();
            nolock=0;
        }
     }
  }
  if(lock>=2) {
    gps_get_position();
    if(sats>=6 && psm_status==0) {
        setGPS_PowerSaveMode();
        psm_status=1;
    }
    nolock=0;
  }
  
  sprintf(data,"%sL%s%d.%05ld,%s%d.%05ld,%ldS%d",data,lat < 0 ? "-" : "",lat_int,lat_dec,lon < 0 ? "-" : "",lon_int,lon_dec,alt,sats);
  
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
  Serial.begin(9600);
  LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
  
  resetGPS();
  
  sensors.begin();
  sensors.getAddress(ds_addr, 0);
  sensors.setResolution(ds_addr, 12);
  
  while (!rf69.init()){
    delay(100);
  }
  
  setupGPS();
  LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF);
  setGps_MaxPerformanceMode();
  
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

void resetGPS() {
  uint8_t set_reset[] = {
    0xB5, 0x62, 0x06, 0x04, 0x04, 0x00, 0xFF, 0x87, 0x00, 0x00, 0x94, 0xF5 };
  sendUBX(set_reset, sizeof(set_reset)/sizeof(uint8_t));
}
void sendUBX(uint8_t *MSG, uint8_t len) {
  Serial.flush();
  Serial.write(0xFF);
  delay(100);
  for(int i=0; i<len; i++) {
    Serial.write(MSG[i]);
  }
}
void setupGPS() {
  //Turning off all GPS NMEA strings apart on the uBlox module
  // Taken from Project Swift (rather than the old way of sending ascii text)
  int gps_set_sucess=0;
  uint8_t setNMEAoff[] = {
    0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, 0x80, 0x25, 0x00, 0x00, 0x07, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0xA9 };
  sendUBX(setNMEAoff, sizeof(setNMEAoff)/sizeof(uint8_t));
  while(!gps_set_sucess)
  {
    sendUBX(setNMEAoff, sizeof(setNMEAoff)/sizeof(uint8_t));
    gps_set_sucess=getUBX_ACK(setNMEAoff);
    delay(500);
  }
  setGPS_DynamicModel3();
  delay(500);
  setGps_MaxPerformanceMode();
  delay(500);
}

void setGPS_DynamicModel3()
{
  int gps_set_sucess=0;
  uint8_t setdm3[] = {
    0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x03,
    0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00,
    0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x2C,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x76 };
  while(!gps_set_sucess)
  {
    sendUBX(setdm3, sizeof(setdm3)/sizeof(uint8_t));
    gps_set_sucess=getUBX_ACK(setdm3);
  }
}
void setGps_MaxPerformanceMode() {
  //Set GPS for Max Performance Mode
  uint8_t setMax[] = { 
    0xB5, 0x62, 0x06, 0x11, 0x02, 0x00, 0x08, 0x00, 0x21, 0x91 }; // Setup for Max Power Mode
  sendUBX(setMax, sizeof(setMax)/sizeof(uint8_t));
}
void setGPS_PowerSaveMode() {
  // Power Save Mode 
  uint8_t setPSM[] = { 
    0xB5, 0x62, 0x06, 0x11, 0x02, 0x00, 0x08, 0x01, 0x22, 0x92 }; // Setup for Power Save Mode (Default Cyclic 1s)
  sendUBX(setPSM, sizeof(setPSM)/sizeof(uint8_t));
}
boolean getUBX_ACK(uint8_t *MSG) {
  uint8_t b;
  uint8_t ackByteID = 0;
  uint8_t ackPacket[10];
  unsigned long startTime = millis();

  // Construct the expected ACK packet    
  ackPacket[0] = 0xB5;	// header
  ackPacket[1] = 0x62;	// header
  ackPacket[2] = 0x05;	// class
  ackPacket[3] = 0x01;	// id
  ackPacket[4] = 0x02;	// length
  ackPacket[5] = 0x00;
  ackPacket[6] = MSG[2];	// ACK class
  ackPacket[7] = MSG[3];	// ACK id
  ackPacket[8] = 0;		// CK_A
  ackPacket[9] = 0;		// CK_B

  // Calculate the checksums
  for (uint8_t ubxi=2; ubxi<8; ubxi++) {
    ackPacket[8] = ackPacket[8] + ackPacket[ubxi];
    ackPacket[9] = ackPacket[9] + ackPacket[8];
  }

  while (1) {

    // Test for success
    if (ackByteID > 9) {
      // All packets in order!
      return true;
    }

    // Timeout if no valid response in 3 seconds
    if (millis() - startTime > 3000) { 
      return false;
    }

    // Make sure data is available to read
    if (Serial.available()) {
      b = Serial.read();

      // Check that bytes arrive in sequence as per expected ACK packet
      if (b == ackPacket[ackByteID]) { 
        ackByteID++;
      } 
      else {
        ackByteID = 0;	// Reset and look again, invalid order
      }

    }
  }
}
uint16_t gps_CRC16_checksum (char *string)
{
  size_t i;
  uint16_t crc;
  uint8_t c;

  crc = 0xFFFF;

  // Calculate checksum ignoring the first two $s
  for (i = 5; i < strlen(string); i++)
  {
    c = string[i];
    crc = _crc_xmodem_update (crc, c);
  }

  return crc;
}
void gps_get_data()
{
  Serial.flush();
  // Clear buf[i]
  for(int i = 0;i<60;i++) 
  {
    buf[i] = 0; // clearing buffer  
  }  
  int i = 0;
  unsigned long startTime = millis();

  while ((i<60) && ((millis() - startTime) < 1000) ) { 
    if (Serial.available()) {
      buf[i] = Serial.read();
      i++;
    }
  }
}
bool _gps_verify_checksum(uint8_t* data, uint8_t len)
{
  uint8_t a, b;
  gps_ubx_checksum(data, len, &a, &b);
  if( a != *(data + len) || b != *(data + len + 1))
    return false;
  else
    return true;
}
void gps_ubx_checksum(uint8_t* data, uint8_t len, uint8_t* cka,
uint8_t* ckb)
{
  *cka = 0;
  *ckb = 0;
  for( uint8_t i = 0; i < len; i++ )
  {
    *cka += *data;
    *ckb += *cka;
    data++;
  }
}
void gps_check_lock()
{
  GPSerror = 0;
  Serial.flush();
  // Construct the request to the GPS
  uint8_t request[8] = {
    0xB5, 0x62, 0x01, 0x06, 0x00, 0x00,
    0x07, 0x16                                                                                                                                                        };
  sendUBX(request, 8);

  // Get the message back from the GPS
  gps_get_data();
  // Verify the sync and header bits
  if( buf[0] != 0xB5 || buf[1] != 0x62 ) {
    GPSerror = 11;
  }
  if( buf[2] != 0x01 || buf[3] != 0x06 ) {
    GPSerror = 12;
  }

  // Check 60 bytes minus SYNC and CHECKSUM (4 bytes)
  if( !_gps_verify_checksum(&buf[2], 56) ) {
    GPSerror = 13;
  }

  if(GPSerror == 0){
    // Return the value if GPSfixOK is set in 'flags'
    if( buf[17] & 0x01 )
      lock = buf[16];
    else
      lock = 0;

    sats = buf[53];
  }
  else {
    lock = 0;
  }
}
void gps_get_position()
{
  GPSerror = 0;
  Serial.flush();
  // Request a NAV-POSLLH message from the GPS
  uint8_t request[8] = {
    0xB5, 0x62, 0x01, 0x02, 0x00, 0x00, 0x03, 0x0A };
  sendUBX(request, 8);

  // Get the message back from the GPS
  gps_get_data();

  // Verify the sync and header bits
  if( buf[0] != 0xB5 || buf[1] != 0x62 )
    GPSerror = 21;
  if( buf[2] != 0x01 || buf[3] != 0x02 )
    GPSerror = 22;

  if( !_gps_verify_checksum(&buf[2], 32) ) {
    GPSerror = 23;
  }

  if(GPSerror == 0) {
    if(sats<4)
    {
      //lat=0;
      //lon=0;
      //alt=0;
    }
    else
    {
      lon = (int32_t)buf[10] | (int32_t)buf[11] << 8 | 
        (int32_t)buf[12] << 16 | (int32_t)buf[13] << 24;
      lat = (int32_t)buf[14] | (int32_t)buf[15] << 8 | 
        (int32_t)buf[16] << 16 | (int32_t)buf[17] << 24;
      alt = (int32_t)buf[22] | (int32_t)buf[23] << 8 | 
        (int32_t)buf[24] << 16 | (int32_t)buf[25] << 24;
    }
    // 4 bytes of latitude/longitude (1e-7)
    lon_int=abs(lon/10000000);
    lon_dec=(labs(lon) % 10000000)/100;
    lat_int=abs(lat/10000000);
    lat_dec=(labs(lat) % 10000000)/100;
    lat/=100;
    lon/=100;


    // 4 bytes of altitude above MSL (mm)

    alt /= 1000; // Correct to meters
  }

}
