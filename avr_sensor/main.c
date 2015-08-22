/**
 * UKHASnet Dumb-Sensor Code by Phil Crump M0DNY
 * Bare metal AVR version by Jon Sowman M0JSN <jon@jonsowman.com>
 * Based on UKHASnet rf69_repeater by James Coxon M6JCX
 */

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <util/delay.h>

#include "ukhasnet-rfm69.h"

#include "nodeconfig.h"

/* Private prototypes */
float get_batt_voltage(void);
int16_t gen_data(char *buf);
void init(void);

/* Global variables local to this compilation unit */
static float battV = 0.0;
static uint32_t count = 1, data_interval = 2;
static uint8_t sequence_id = 97; // 'a'
static char databuf[64];

/**
 * Measure the battery voltage.
 * @returns The voltage of the battery in Volts
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
    PRR |= _BV(PRADC);

    return (float)(res * 0.001074219 * BATTV_SCALEFACTOR * BATTV_FUDGE);
}

int16_t gen_data(char *buf)
{
    int8_t temp = 0;

#ifdef LOCATION_STRING
    if(sequence_id=='a' || sequence_id=='z') {
        sprintf(buf, "%u%cL%s", NUM_REPEATS, sequence_id, LOCATION_STRING);
    } else {
        sprintf(buf, "%u%c", NUM_REPEATS, sequence_id);
    }
#else
    sprintf(buf, "%u%c", NUM_REPEATS, sequence_id);
#endif

    temp = rf69_readTemp();
    sprintf(buf, "%sT%i.0", buf, temp);

    // Battery Voltage
#if ENABLE_BATTV_SENSOR == 1
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

    return sprintf(buf, "%s[%s]", buf, NODE_ID);
}

void init(void)
{
    int16_t packet_len;

    /* Turn off peripherals that we don't use */
    PRR |= _BV(PRTWI) | _BV(PRTIM2) | _BV(PRTIM1) | _BV(PRTIM0) | _BV(PRUSART0);

    while(!rfm69_init())
        _delay_ms(100);

    packet_len = gen_data(databuf);
    rf69_send((uint8_t*)databuf, packet_len, RFM_POWER);
}

int main(void)
{
    int16_t packet_len;

    init();

    rf69_setMode(RFM69_MODE_SLEEP);

    /* Initial data interval = BEACON_INTERVAL since count = 0 */
    data_interval = BEACON_INTERVAL;

    while(1)
    {
        count++;
        /* Enable the watchdog and sleep for 8 seconds */
        wdt_enable(WDTO_8S);
        WDTCSR |= (1 << WDIE);
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_enable();
        sei();
        sleep_cpu();
        sleep_disable();

        if(count >= data_interval)
        {
            sequence_id++;

            /* Wrap the seqid */
            if(sequence_id > 122)
                sequence_id = 98; //'b'

            packet_len = gen_data(databuf);
            rf69_send((uint8_t*)databuf, packet_len, RFM_POWER);

            data_interval = BEACON_INTERVAL + count;
        }
    }

    return 0;
}

/**
 * Watchdog interrupt
 */
ISR(WDT_vect)
{
    wdt_disable();
}
