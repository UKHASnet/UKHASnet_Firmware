/**
 * UKHASnet Repeater Code by Phil Crump M0DNY
 * Bare metal AVR version by Jon Sowman M0JSN <jon+github@jonsowman.com>
 * Based on UKHASnet rf69_repeater by James Coxon M6JCX
 *
 * Uses the ukhasnet-rfm69 library. Please see README.me in the git repository
 * for instructions to get and use the library.
 * https://github.com/UKHASnet/ukhasnet-rfm69
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

/* Zombie mode */
typedef enum zombie_mode_t {MODE_NORMAL, MODE_ZOMBIE} zombie_mode_t;

static char packet_buf[65]; /* 64+1 to allow for sprintf-added NULL (not transmitted) */

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

/**
 * If ENABLE_ZOMBIE_MODE is defined in nodeconfig.h, this function should be
 * called regularly to bring the node in and out of zombie mode as a function
 * of battery voltage. ZOMBIE_THRESHOLD is the zombie mode threshold battery
 * voltage in Volts, define this also in nodeconfig.h.
 */
bool _use_zombie_mode(float battery_voltage)
{
#ifdef ENABLE_ZOMBIE_MODE

    static zombie_mode_t zombie_mode = MODE_NORMAL;

    if(zombie_mode == MODE_ZOMBIE)
    {
        /* If the battery voltage is higher than (threshold + hysteresis), exit zombie mode */
        if(battery_voltage > (ZOMBIE_THRESHOLD + ZOMBIE_HYST))
        {
            zombie_mode = MODE_NORMAL;
            return false;
        }
        else
        {
            zombie_mode = MODE_ZOMBIE;
            return true;
        }
    }
    else /* zombie_mode == MODE_NORMAL */
    {
        /* If the current battery voltage is less than threshold, enter zombie mode! */
        if(battery_voltage < ZOMBIE_THRESHOLD)
        {
            zombie_mode = MODE_ZOMBIE;
            return true;
        }
        else
        {
            zombie_mode = MODE_NORMAL;
            return false;
        }
    }

#else

    (void) battery_voltage;
    return false;

#endif /* ENABLE_ZOMBIE_MODE */
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

    /* If zombie mode is enabled, put the current zombie status in telem */
#ifdef ENABLE_ZOMBIE_MODE
    sprintf(buf, "%sZ%d", buf, _use_zombie_mode(battV));
#endif /* ENABLE_ZOMBIE_MODE */

    return sprintf(buf, "%s[%s]", buf, NODE_ID);
}

static void _power_down(const uint8_t wdg_value)
{
    /* Enable the watchdog and sleep */
    wdt_enable(wdg_value);
    WDTCSR |= (1 << WDIE);
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sei();
    sleep_cpu();
    sleep_disable();
}

/**
 * Get a random number between an upper and lower limit. The random number
 * generation itself is rand(), which must be supported in the platform of 
 * your choice.
 * @param lower The lower boundary
 * @param upper The upper boundary
 * @returns The resulting random number between lower and upper.
 */
static uint16_t getRandBetween(const uint16_t lower, const uint16_t upper)
{
    return (uint16_t)(((double)rand() / ((double)RAND_MAX + 1) * (upper-lower))+lower);
}

static void _repeat_packet(char *packet_buffer, uint8_t packet_length)
{
    uint8_t i, delaytime;
    uint8_t *end_bracket;

    /* Check Packet TTL */
    if(packet_buffer[0] == '0')
    {
        /* TTL Expired */
        return;
    }
        
    /* Check packet length (including our NODE_ID and a comma) */
    if((packet_length + strlen(NODE_ID) + 1) > 64)
    {
        /* Repeated packet will be too long, drop it */
        return;
    }

    /* Check for our NODE ID */
    if(memmem(packet_buffer, packet_length, NODE_ID, strlen(NODE_ID)) != NULL)
    {
        /* We've already handled the packet, drop it */
        return;
    }

    /* Decrement TTL */
    packet_buffer[0]--;

    /* Find the brackets */
    end_bracket = memchr(packet_buffer, ']', packet_length);
    if(end_bracket == NULL)
    {
        /* End bracket not found */
        return;
    }

    /* Append our NODE_ID */
    sprintf((char *)end_bracket, ",%s]", NODE_ID);

    /* Update packet length for NODE_ID and comma */
    packet_length += strlen(NODE_ID) + 1;

    /* Random delay to try and avoid packet collision */
    delaytime = getRandBetween(5u, 80u);
    for(i = 0; i < delaytime; i++)
        _delay_ms(10);

    /* Now send the repeated packet */
    rf69_send((rfm_reg_t *)packet_buffer, packet_length, RFM_POWER);
}

/**
 * Call init functions and then run forever.
 */
int main(void)
{
    uint32_t wakecount = 0;
    uint32_t next_tx_wakecount;
    uint32_t tx_wakecount_interval;
    uint8_t sequence_id = 'a';

    bool packet_received;
    uint8_t i, packet_len;
    int16_t lastrssi;

    /* Turn off peripherals that we don't use */
    PRR |= _BV(PRTWI) | _BV(PRTIM2) | _BV(PRTIM1) | _BV(PRTIM0) | _BV(PRUSART0);

    /* Initialise the RFM69 */
    while(rf69_init() != RFM_OK)
        _delay_ms(100);

    /* Generate and transmit a packet */
    packet_len = gen_data(packet_buf, &sequence_id);
    rf69_send((rfm_reg_t *)packet_buf, packet_len, RFM_POWER);

    /* Set transmit interval (rounded up to a multiple of 8s watchdog cycles) */
    tx_wakecount_interval = (BEACON_INTERVAL / 8) + !!(BEACON_INTERVAL % 8);

    /* Set next Transmit time */
    next_tx_wakecount = wakecount + tx_wakecount_interval;

    while(1)
    {
        if(_use_zombie_mode(get_batt_voltage()))
        {
            /* Battery Voltage Low - Zombie Mode, ignore all repeating
             * functionality of the node 
             * Low Power Sleep for 8 seconds
             */
            rf69_set_mode(RFM69_MODE_SLEEP);
            _power_down(WDTO_8S);
        }
        else
        {
            /* Set RFM69 into Receive Mode */
            rf69_set_mode(RFM69_MODE_RX);

            /* This loop is calculated to take roughly 8 seconds, i.e. 255
             * iterations with a 30ms delay */
            for(i = 0; i < 0xFF; i++)
            {
                /* Poll the RFM69 for a received packet */
                rf69_receive((rfm_reg_t *)packet_buf, &packet_len, &lastrssi, &packet_received);

                if(packet_received)
                {
                    /* Repeat the received packet */
                    _repeat_packet(packet_buf, packet_len);
                }
                else
                {
                    /* Sleep for 30ms */
                    _power_down(WDTO_30MS);
                }
            }
        }

        /* Check if we're due to transmit a beacon */
        if(wakecount == next_tx_wakecount)
        {
            /* Increment sequence id */
            if(sequence_id == 'z')
                sequence_id = 'b';
            else
                sequence_id++;

            /* Generate and transmit a packet */
            packet_len = gen_data(packet_buf, &sequence_id);
            rf69_send((rfm_reg_t *)packet_buf, packet_len, RFM_POWER);

            /* Set next Transmit time */
            next_tx_wakecount = wakecount + getRandBetween(tx_wakecount_interval, tx_wakecount_interval+2);
        }

        wakecount++;
    }

    return 0;
}

/**
 * Watchdog interrupt, we have just woken from sleep. Kill the watchdog and
 * then exit the ISR, main execution continues.
 */
ISR(WDT_vect)
{
    wdt_disable();
}
