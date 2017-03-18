/**
 * UKHASnet Gateway Code by Phil Crump M0DNY
 * Bare metal AVR version by David Brooke G6GZH <github@dbrooke.me.uk>
 * Based on UKHASnet rf69_repeater by James Coxon M6JCX
 *
 * Uses the ukhasnet-rfm69 library. Please see README.me in the git repository
 * for instructions to get and use the library.
 * https://github.com/UKHASnet/ukhasnet-rfm69
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <util/delay.h>

#include "ukhasnet-rfm69.h"
#include "nodeconfig.h"
#include "uart.h"

/**
 * Initialise internal and external peripherals, including reading config and
 * setting power savings modes.
 */
void init(void)
{
    /* Turn off peripherals that we don't use */
    PRR |= _BV(PRTWI) | _BV(PRTIM2) | _BV(PRTIM1) | _BV(PRTIM0);

    while(rf69_init() != RFM_OK)
        _delay_ms(100);
}

/**
 * Call init functions and then run forever.
 */
int main(void)
{
    bool ispacket;
    int16_t lastrssi;
    rfm_reg_t len;
    char packet_buf[65], *p;

    init();

    uart_init();
    stdout = &mystdout;

    while(1) {
        _delay_ms(30);
        rf69_receive((rfm_reg_t *)packet_buf, &len, &lastrssi, &ispacket);

        /* The boolean variable 'ispacket' will be true if rf69_receive
         * tells us there is a packet waiting in the RFM69 receive buffer
         */
        if( ispacket )
        {
            /* Find end of packet */
            p = memchr(packet_buf, ']', len);

            /* If it is a valid packet then send on serial */
            if (p != NULL)
            {
                *++p = '\0';
                printf("rx: %s|%d\n", packet_buf, lastrssi);
            }
        }
    }
    return 0;
}

