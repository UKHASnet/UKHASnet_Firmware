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
float get_batt_voltage(void);
int16_t gen_data(char *buf);
void init(void);

/* Global variables local to this compilation unit */
static float battV = 0.0;
static uint8_t n;
static uint32_t count = 1, data_interval = 2;
static uint8_t sequence_id = 97; // 'a'
static char databuf[64], string_end[] = "]";

/*#ifdef ENABLE_BATTV_SENSOR*/
 /*float sampleBattv() {*/
   /*// External 5:1 Divider*/
   /*return ((float)analogRead(BATTV_PIN)*1.1*5*BATTV_FUDGE)/1023.0;*/
 /*}*/
/*#endif*/

/**
 * Measure the battery voltage
 * @returns The voltage of the battery in mV
 */
float get_batt_voltage(void)
{
    uint16_t res;

    /* Power up the ADC */
    PRR &= ~(_BV(PRADC));

    /* Set channel to PC0 */
    ADMUX = BATTV_PIN;

    /* Set internal 1V1 reference */
    ADMUX |= _BV(REFS1) | _BV(REFS0);

    /* Get 125kHz ADC clock from a 1MHz core clock by dividing by 8 */
    ADCSRA |= _BV(ADPS1) | _BV(ADPS0);

    /* Enable ADC */
    ADCSRA |= _BV(ADEN);

    /* Start conv, wait until done, get result */
    ADCSRA |= _BV(ADSC);
    while(!(ADCSRA & _BV(ADIF)));
    res = ADC;

    /* Power down the ADC */
    ADCSRA &= ~_BV(ADEN);

    return (float)(res * 0.001074219 * BATTV_SCALEFACTOR * BATTV_FUDGE);
}

int16_t gen_data(char *buf)
{
    char* tempStr;
    char tempStrA[12]; //make buffer large enough for 7 digits
    int8_t temp = 0;

#ifdef LOCATION_STRING
    if(sequence_id=='a' || sequence_id=='z') {
        sprintf(buf, "%c%cL%s", num_repeats, sequence_id, LOCATION_STRING);
    } else {
        sprintf(buf, "%c%c", num_repeats, sequence_id);
    }
#else
    sprintf(buf, "%c%c", num_repeats, sequence_id);
#endif

    temp = rf69_readTemp();
    sprintf(buf, "%sT%i.0", buf, temp);

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
    battV = get_batt_voltage();
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

    init();

    while(1)
    {
        count++;

        rf69_setMode(RFM69_MODE_SLEEP);
        /* TODO: This should sleep properly using the watchdog */
        _delay_ms(1000);

        if(count >= data_interval)
        {
            sequence_id++;

            /* Wrap the seqid */
            if(sequence_id > 122)
                sequence_id = 98; //'b'

            packet_len = gen_data(databuf);
            rf69_send((uint8_t*)databuf, packet_len, rfm_power);

            data_interval = BEACON_INTERVAL + count;
        }
    }

    return 0;
}
