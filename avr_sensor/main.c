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


static char databuf[64];

/**
 * Measure the battery voltage.
 * @returns The voltage of the battery in Volts
 */
static float get_batt_voltage(void)
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

    /* 1.1V / 10bit = 0.001074219 */
    return (float)(res * 0.001074219 * BATTV_SCALEFACTOR * BATTV_FUDGE);
}

static int16_t gen_data(char *buf, uint8_t *sequence_id)
{
    int8_t temp = 0;

#ifdef LOCATION_STRING
    if(*sequence_id=='a' || *sequence_id=='z') {
        sprintf(buf, "%u%cL%s", NUM_REPEATS, *sequence_id, LOCATION_STRING);
    } else {
        sprintf(buf, "%u%c", NUM_REPEATS, *sequence_id);
    }
#else
    sprintf(buf, "%u%c", NUM_REPEATS, *sequence_id);
#endif

    rf69_read_temp(&temp);
    sprintf(buf, "%sT%i.0", buf, temp);

    /* Battery Voltage */
#if ENABLE_BATTV_SENSOR == 1
	static float battV = 0.0;
    battV = get_batt_voltage();
    char* battStr;
    char tempStrB[14]; /* make buffer large enough for 7 digits */
    battStr = dtostrf(battV,7,2,tempStrB);
    while( (strlen(battStr) > 0) && (battStr[0] == 32) )
    {
        strcpy(battStr,&battStr[1]);
    }
    sprintf(buf, "%sV%s", buf, battStr);
#endif

    return sprintf(buf, "%s[%s]", buf, NODE_ID);
}

int main(void)
{
	uint32_t wakecount = 0;
	uint32_t next_tx_wakecount;
	uint32_t tx_wakecount_interval;
    int16_t packet_len;
	uint8_t sequence_id = 'a';

    /* Turn off peripherals that we don't use */
    PRR |= _BV(PRTWI) | _BV(PRTIM2) | _BV(PRTIM1) | _BV(PRTIM0) | _BV(PRUSART0);

    /* Initialise the RFM69 */
    while(rf69_init() != RFM_OK)
        _delay_ms(100);

    /* Generate and transmit a packet */
    packet_len = gen_data(databuf, &sequence_id);
    rf69_send((rfm_reg_t *)databuf, packet_len, RFM_POWER);

    /* Set transmit interval (rounded up to a multiple of 8s watchdog cycles) */
    tx_wakecount_interval = (BEACON_INTERVAL / 8) + !!(BEACON_INTERVAL % 8);

    /* Set next Transmit time */
    next_tx_wakecount = wakecount + tx_wakecount_interval;

    while(1)
    {
        /* Enable the watchdog and sleep for 8 seconds */
        wdt_enable(WDTO_8S);
        WDTCSR |= (1 << WDIE);
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_enable();
        sei();
        sleep_cpu();
        sleep_disable();

        if(wakecount == next_tx_wakecount)
        {
            /* Increment sequence id */
            if(sequence_id == 'z')
                sequence_id = 'b';
            else
            	sequence_id++;

    		/* Generate and transmit a packet */
            packet_len = gen_data(databuf, &sequence_id);
            rf69_send((rfm_reg_t *)databuf, packet_len, RFM_POWER);

            /* Set next Transmit time */
            next_tx_wakecount = wakecount + tx_wakecount_interval;
        }

        wakecount++;
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