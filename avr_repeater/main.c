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
#include "ukhasnet-rfm69-config.h"
#include "nodeconfig.h"

/* Zombie mode */
typedef enum zombie_mode_t {MODE_NORMAL, MODE_ZOMBIE} zombie_mode_t;

/* Global variables local to this compilation unit */
static int16_t packet_len;
static float battV = 0.0;
static uint32_t count = 1, data_interval = 2;
static uint8_t sequence_id = 'a';
static char databuf[64];
static rfm_reg_t buf[64], len;
static int16_t lastrssi;
static zombie_mode_t zombie_mode = MODE_NORMAL;

/* Private prototypes */
float get_batt_voltage(void);
int16_t gen_data(char *buf);
void init(void);
uint16_t getRandBetween(const uint16_t lower, const uint16_t upper);
void enableRepeat(void);
void loop(void);
void sendPacket(void);

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

    rf69_readTemp(&temp);
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

/**
 * Initialise internal and external peripherals, including reading config and
 * setting power savings modes.
 */
void init(void)
{
    /* Turn off peripherals that we don't use */
    PRR |= _BV(PRTWI) | _BV(PRTIM2) | _BV(PRTIM1) | _BV(PRTIM0) | _BV(PRUSART0);

    while(!rfm69_init())
        _delay_ms(100);
    sendPacket();

    enableRepeat();
}

/**
 * A wrapper function to generate a packet using gen_data() and then transmit
 * it using the RFM69 radio.
 */
void sendPacket(void)
{
    if(sequence_id > 'z')
    {
        sequence_id = 'b';
    }
    packet_len = gen_data(databuf);
    rf69_send((rfm_reg_t *)databuf, packet_len, RFM_POWER);
    sequence_id++;
    wdt_reset();
    _delay_ms(1000);
    wdt_reset();
}

/**
 * If ENABLE_ZOMBIE_MODE is defined in nodeconfig.h, this function should be
 * called regularly to bring the node in and out of zombie mode as a function
 * of battery voltage. ZOMBIE_THRESHOLD is the zombie mode threshold battery
 * voltage in Volts, define this also in nodeconfig.h.
 */
void enableRepeat(void)
{
#ifdef ENABLE_ZOMBIE_MODE
    if(battV > ZOMBIE_THRESHOLD) {
        rf69_setMode(RFM69_MODE_RX);
        zombie_mode = MODE_NORMAL;
#ifdef SENSITIVE_RX
        rf69_SetLnaMode(RF_TESTLNA_SENSITIVE);
#endif /* SENSITIVE_RX */
    } else {
        rf69_setMode(RFM69_MODE_SLEEP);
        zombie_mode = MODE_ZOMBIE;
    }
#else
    rf69_setMode(RFM69_MODE_RX);
    zombie_mode = MODE_NORMAL;
#ifdef SENSITIVE_RX
    rf69_SetLnaMode(RF_TESTLNA_SENSITIVE);
#endif /* SENSITIVE_RX */
#endif /* ENABLE_ZOMBIE_MODE */
}

/**
 * Call init functions and then run forever.
 */
int main(void)
{
    init();
    rf69_setMode(RFM69_MODE_SLEEP);

    /* Initial data interval = BEACON_INTERVAL since count = 0 */
    data_interval = BEACON_INTERVAL;

    while(1)
    {
        _delay_ms(8000);
        loop();
    }

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
    uint8_t i;

    count++;
    wdt_reset();

    if( zombie_mode == MODE_NORMAL )
    {
        rf69_setMode(RFM69_MODE_RX);
        for(i = 0; i < 255; i++) {
            wdt_reset();
            //LowPower.idle(SLEEP_30MS, ADC_OFF, TIMER2_OFF, TIMER1_OFF, 
            //  TIMER0_OFF, SPI_OFF, USART0_ON, TWI_OFF);
            _delay_ms(30);
            rf69_receive(buf, &len, &lastrssi, &ispacket);
            if(ispacket)
            {

/*
#ifdef ENABLE_UART_OUTPUT
                  rf69_lastRssi(&lastrssi);
                  for (j=0; j<len; j++) {
                  Serial.print((char)buf[j]);
                  if(buf[j]==']') break;
                  }
                  Serial.print("|");
                  Serial.println(rx_rssi);
#endif
*/

                // Find end of packet & start of repeaters
                uint8_t end_bracket = -1, start_bracket = -1;        
                for (int k=0; k<len; k++)
                {
                    if (buf[k] == '[')
                    {
                        start_bracket = k;
                    }
                    else if (buf[k] == ']')
                    {
                        end_bracket = k;
                        buf[k+1] = '\0';
                        break;
                    }
                }

                // Need to take the recieved buffer and decode it and 
                // add a reference 
                if (buf[0] > '0' && end_bracket != -1 && strstr((const char *)&buf[start_bracket], NODE_ID) == NULL)
                {
                    // Reduce the repeat value
                    buf[0]--;

                    // Add the repeater ID
                    packet_len = end_bracket + sprintf((char *)&buf[end_bracket], ",%s]", NODE_ID);
                    uint16_t delayValue = getRandBetween(5u, 80u);
                    for (uint8_t j = 0; j < delayValue;j++)
                    {
                        //random delay to try and avoid packet collision
                        _delay_ms(10);
                    }

                    rf69_send((rfm_reg_t *)buf, packet_len, RFM_POWER);
/*
#ifdef ENABLE_UART_OUTPUT
                      for (j=0; j<packet_len; j++) {
                      if(buf[j]==']'){
                      Serial.println((char)buf[j]);
                      break;
                      }
                      Serial.print((char)buf[j]);
                      }
#endif
*/
                }
            }
        }
    }
    else
    {
        // Battery Voltage Low - Zombie Mode, ignore all repeating
        // functionality of the node

        // Low Power Sleep for 8 seconds
        rf69_setMode(RFM69_MODE_SLEEP);
        _delay_ms(8000u);
    }

    if (count >= data_interval)
    {
        sequence_id++;

        // Reset the sequence id if we've reached 'z'
        if(sequence_id > 'z')
            sequence_id = 'b';

        packet_len = gen_data(databuf);
        rf69_send((rfm_reg_t *)databuf, packet_len, RFM_POWER);
/*
#ifdef ENABLE_UART_OUTPUT
        // Print own Beacon Packet
        for (j=0; j<packet_len; j++)
        {
        if(data[j]==']') // Check for last char in packet
        {
        Serial.println(data[j]);
        break;
        }
        Serial.print(data[j]);
        }
#endif
*/

        data_interval = getRandBetween((BEACON_INTERVAL/8), 
                (BEACON_INTERVAL/8)+2) + count;
#ifdef ENABLE_ZOMBIE_MODE
        if(battV > ZOMBIE_THRESHOLD && zombie_mode == MODE_ZOMBIE)
        {
            rf69_setMode(RFM69_MODE_RX);
            zombie_mode = MODE_NORMAL;
#ifdef SENSITIVE_RX
            rf69_SetLnaMode(RF_TESTLNA_SENSITIVE);
#endif
        }
        else if (battV < ZOMBIE_THRESHOLD && zombie_mode == MODE_NORMAL)
        {
            rf69_setMode(RFM69_MODE_SLEEP);
            zombie_mode = MODE_NORMAL;
        }
#endif
    }
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
 * Watchdog interrupt
 */
ISR(WDT_vect)
{
    wdt_disable();
}
