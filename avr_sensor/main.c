/**
 * UKHASnet Dumb-Sensor Code by Phil Crump M0DNY
 * Bare metal AVR version by Jon Sowman M0JSN <jon@jonsowman.com>
 * Based on UKHASnet rf69_repeater by James Coxon M6JCX
 */

#include <avr/io.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <util/delay.h>

#include "RFM69Config.h"
#include "RFM69.h"
#include "nodeconfig.h"

/* Private prototypes */
int16_t gen_data(char *buf);
void init(void);

/* Global variables local to this compilation unit */
static float battV = 0.0;
static uint8_t n;
static uint32_t count = 1, data_interval = 2;
static uint8_t data_count = 97; // 'a'
static char databuf[64], string_end[] = "]";

/*#ifdef ENABLE_BATTV_SENSOR*/
 /*float sampleBattv() {*/
   /*// External 5:1 Divider*/
   /*return ((float)analogRead(BATTV_PIN)*1.1*5*BATTV_FUDGE)/1023.0;*/
 /*}*/
/*#endif*/

int16_t gen_data(char *buf)
{
    char* tempStr;
    char tempStrA[12]; //make buffer large enough for 7 digits
    float temp = 0.0;

#ifdef LOCATION_STRING
    if(data_count=='a' || data_count=='z') {
        sprintf(buf, "%c%cL%s", num_repeats, data_count, LOCATION_STRING);
    } else {
        sprintf(buf, "%c%c", num_repeats, data_count);
    }
#else
    sprintf(buf, "%c%c", num_repeats, data_count);
#endif

    tempStr = dtostrf(temp,6,1,tempStrA);
    while( (strlen(tempStr) > 0) && (tempStr[0] == 32) )
    {
        strcpy(tempStr,&tempStr[1]);
    }
    sprintf(buf, "%sT%s", buf, tempStr);

    // Humidity (DHT22)

#ifdef DHT22
    float humid = DHT.humidity;

    tempStr = dtostrf(humid,6,1,tempStrA);
    while( (strlen(tempStr) > 0) && (tempStr[0] == 32) )
    {
        strcpy(tempStr,&tempStr[1]);
    }
    sprintf(buf, "%sH%s", buf, tempStr);
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
    sprintf(buf, "%sV%s", buf, battStr);
#endif

    return sprintf(buf, "%s[%s]", buf, id);
}

void init(void)
{
    int16_t packet_len;

    while (!rf69_init())
        _delay_ms(100);

    packet_len = gen_data(databuf);
    rf69_send((uint8_t*)databuf, packet_len, rfm_power);
}

int main(void)
{
    int16_t packet_len;

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

            packet_len = gen_data(databuf);
            rf69_send((uint8_t*)databuf, packet_len, rfm_power);

            data_interval = BEACON_INTERVAL;
        }
    }

    return 0;
}
