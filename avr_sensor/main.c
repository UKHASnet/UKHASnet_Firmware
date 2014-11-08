/**
 * UKHASnet Dumb-Sensor Code by Phil Crump M0DNY
 * Bare metal AVR version by Jon Sowman M0JSN <jon@jonsowman.com>
 * Based on UKHASnet rf69_repeater by James Coxon M6JCX
 */

#include <avr/io.h>
#include <string.h>
#include <util/delay.h>

#include "RFM69Config.h"
#include "RFM69.h"
#include "nodeconfig.h"

/* Private prototypes */
int gen_data(void);
void init(void);

/* Global variables local to this compilation unit */
static float battV=0.0;
static uint8_t n;
static uint32_t count = 1, data_interval = 2;
static uint8_t data_count = 97; // 'a'
static char data[64], string_end[] = "]";

/*#ifdef ENABLE_BATTV_SENSOR*/
 /*float sampleBattv() {*/
   /*// External 5:1 Divider*/
   /*return ((float)analogRead(BATTV_PIN)*1.1*5*BATTV_FUDGE)/1023.0;*/
 /*}*/
/*#endif*/

int gen_data(char *buf)
{
    char* tempStr;
    char tempStrA[12]; //make buffer large enough for 7 digits

#ifdef LOCATION_STRING
    if(data_count=='a' or data_count=='z') {
        sprintf(data, "%c%cL%s", num_repeats, data_count, LOCATION_STRING);
    } else {
        sprintf(data, "%c%c", num_repeats, data_count);
    }
#else
    sprintf(data, "%c%c", num_repeats, data_count);
#endif

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

void init(void)
{
    while (!rf69_init())
    {
        _delay_ms(100);
    }

    int packet_len = gen_Data();
    rf69.send((uint8_t*)data, packet_len, rfm_power);
}

int main(void)
{
    int packet_len;

    while(1)
    {
        count++;

        rf69_setMode(RFM69_MODE_SLEEP);

        if (count >= data_interval)
        {
            data_count++;

            /* Wrap the seqid */
            if(data_count > 122)
                data_count = 98; //'b'

            packet_len = gen_data();
            rf69_send((uint8_t*)data, packet_len, rfm_power);

            data_interval = random((BEACON_INTERVAL/8), (BEACON_INTERVAL/8)+2) + count;
        }
    }

    return 0;
}
