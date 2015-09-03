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

/* Global variables local to this compilation unit */
static float battV = 0.0;
static uint32_t count = 1, data_interval = 2;
static char sequence_id = 'a';
static char packet_buf[64];
static zombie_mode_t zombie_mode = MODE_NORMAL;

/* Private prototypes */
float get_batt_voltage(void);
int16_t gen_data(char* buf);
void init(void);
uint16_t getRandBetween(const uint16_t lower, const uint16_t upper);
void zombieMode(void);
void loop(void);
void sendPacket(void);

/**
 * Initialise internal and external peripherals, including reading config and
 * setting power savings modes.
 */
void init(void)
{
    /* Turn off peripherals that we don't use */
    PRR |= _BV(PRTWI) | _BV(PRTIM2) | _BV(PRTIM1) | _BV(PRTIM0) | _BV(PRUSART0);

    while(rf69_init() != RFM_OK)
        _delay_ms(100);

    /* Transmit an initial packet */
    sendPacket();
}

/**
 * A wrapper function to generate a packet using gen_data() and then transmit
 * it using the RFM69 radio.
 */
void sendPacket(void)
{
    uint16_t packet_len;

    if(sequence_id > 'z')
    {
        sequence_id = 'b';
    }
    packet_len = gen_data(packet_buf);
    rf69_send((rfm_reg_t *)packet_buf, packet_len, RFM_POWER);
    sequence_id++;
}

/**
 * If ENABLE_ZOMBIE_MODE is defined in nodeconfig.h, this function should be
 * called regularly to bring the node in and out of zombie mode as a function
 * of battery voltage. ZOMBIE_THRESHOLD is the zombie mode threshold battery
 * voltage in Volts, define this also in nodeconfig.h.
 */
void zombieMode(void)
{
#ifdef ENABLE_ZOMBIE_MODE
    if(battV > (ZOMBIE_THRESHOLD + ZOMBIE_HYST) && zombie_mode == MODE_ZOMBIE)
    {
        rf69_set_mode(RFM69_MODE_RX);
        zombie_mode = MODE_NORMAL;
#ifdef SENSITIVE_RX
        rf69_set_lna_mode(RF_TESTLNA_SENSITIVE);
#endif /* SENSITIVE_RX */
    }
    else if(battV < ZOMBIE_THRESHOLD && zombie_mode == MODE_NORMAL)
    {
        rf69_set_mode(RFM69_MODE_SLEEP);
        zombie_mode = MODE_ZOMBIE;
    }
#else
    rf69_set_mode(RFM69_MODE_RX);
    zombie_mode = MODE_NORMAL;
#ifdef SENSITIVE_RX
    rf69_set_lna_mode(RF_TESTLNA_SENSITIVE);
#endif /* SENSITIVE_RX */
#endif /* ENABLE_ZOMBIE_MODE */
}

/**
 * Call init functions and then run forever.
 */
int main(void)
{
    init();
    rf69_set_mode(RFM69_MODE_SLEEP);

    /* Initial data interval = BEACON_INTERVAL since count = 0 */
    data_interval = BEACON_INTERVAL/8;

    while(1)
        loop();

    return 0;
}

/**
 * Contents of this function are called every iteration of the infinite main
 * loop. We should do houskeeping, then transmit/receive as and when it is
 * necessary.
 */
void loop(void)
{
    bool ispacket;
    uint8_t i, j, k, start_bracket, end_bracket;
    uint16_t delaytime, packet_len;
    int16_t lastrssi;
    rfm_reg_t len;

    count++;
    wdt_reset();

    if( zombie_mode == MODE_NORMAL )
    {
        rf69_set_mode(RFM69_MODE_RX);

        /* This loop is calculated to take roughly 8 seconds, i.e. 255
         * iterations with a 30ms delay */
        for(i = 0; i < 255; i++) {
            wdt_reset();
            _delay_ms(30);
            rf69_receive((rfm_reg_t *)packet_buf, &len, &lastrssi, &ispacket);

            /* The boolean variable 'ispacket' will be true if rf69_receive
             * tells us there is a packet waiting in the RFM69 receive buffer
             */
            if( ispacket )
            {
                /* Find end of packet & start of repeaters */
                end_bracket = 255;
                start_bracket = 255;
                /* TODO: This should be replaced with strstr() */
                for (k = 0; k < len; k++)
                {
                    if (packet_buf[k] == '[')
                    {
                        start_bracket = k;
                    }
                    else if (packet_buf[k] == ']')
                    {
                        end_bracket = k;
                        packet_buf[k+1] = '\0';
                        break;
                    }
                }

                /* Need to take the recieved buffer and decode it and 
                 * add a reference, under 3 conditions:
                 * 1) The number of 'hops' remaining is > 0
                 * 2) We found an end bracket in the packet, i.e. it is not
                 * malformed in some way
                 * 3) We have not /already/ repeated this packet
                 * */
                if (packet_buf[0] > '0' 
                        && end_bracket != 255
                        && strstr((const char *)&packet_buf[start_bracket], NODE_ID) == NULL)
                {
                    /* Reduce the TTL (hops) */
                    packet_buf[0]--;

                    /* Add the repeater ID */
                    packet_len = end_bracket + sprintf((char *)&packet_buf[end_bracket], ",%s]", NODE_ID);

                    /* Random delay to try and avoid packet collision */
                    delaytime = getRandBetween(5u, 80u);
                    for(j = 0; j < delaytime; j++)
                        _delay_ms(10);

                    /* Now send the repeated packet */
                    rf69_send((rfm_reg_t *)packet_buf, packet_len, RFM_POWER);
                }
            }
        }
    }
    else
    {
        /* Battery Voltage Low - Zombie Mode, ignore all repeating
         * functionality of the node 
         * Low Power Sleep for 8 seconds
         */
        rf69_set_mode(RFM69_MODE_SLEEP);

        /* Enable the watchdog and sleep for 8 seconds */
        wdt_enable(WDTO_8S);
        WDTCSR |= (1 << WDIE);
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_enable();
        sei();
        sleep_cpu();
        sleep_disable();
    }

    /* Time to send a beacon? */
    if(count >= data_interval)
    {
        /* Send a packet */
        sendPacket();

        /* When will we send the next beacon? */
        data_interval = getRandBetween((BEACON_INTERVAL/8), 
                (BEACON_INTERVAL/8)+2) + count;

        /* Check if we need to enter or leave zombie mode */
        zombieMode();
    }
}

/**
 * Given a char buffer, generate a UKHASnet packet and return the length of
 * the packet written to the supplied buffer.
 * @param buf The char buffer into which to write the packet
 * @returns The length of the packet that was written
 */
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

    /* Read the temperature and if successful, add to packet */
    if( rf69_read_temp(&temp) == RFM_OK )
        sprintf(buf, "%sT%i.0", buf, temp);

    /* Battery Voltage */
#if ENABLE_BATTV_SENSOR == 1
    battV = get_batt_voltage();
    char* battStr;
    char tempStrB[14]; /* make buffer large enough for 7 digits */
    battStr = dtostrf(battV,7,2,tempStrB);
    /* Remove leading spaces by incrementing the pointer */
    while( *battStr++ == ' ' );
    sprintf(buf, "%sV%s", buf, battStr);
#endif

    /* If zombie mode is enabled, put the current zombie status in telem */
#ifdef ENABLE_ZOMBIE_MODE
    sprintf(buf, "%sZ%d", buf, zombie_mode);
#endif /* ENABLE_ZOMBIE_MODE */

    return sprintf(buf, "%s[%s]", buf, NODE_ID);
}

/**
 * Measure the battery voltage as measured by a potential divider on BATTV_PIN
 * defined in nodeconfig.h. BATTV_SCALEFACTOR should also be set appropriately
 * in this file.
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

/**
 * Get a random number between an upper and lower limit. The random number
 * generation itself is rand(), which must be supported in the platform of 
 * your choice.
 * @param lower The lower boundary
 * @param upper The upper boundary
 * @returns The resulting random number between lower and upper.
 */
uint16_t getRandBetween(const uint16_t lower, const uint16_t upper)
{
    return (uint16_t)(((double)rand() / ((double)RAND_MAX + 1) * (upper-lower))+lower);
}

/**
 * Watchdog interrupt, we have just woken from sleep. Kill the watchdog and
 * then exit the ISR, main execution continues.
 */
ISR(WDT_vect)
{
    wdt_disable();
}
