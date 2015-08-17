/**
 * spi_conf.c
 *
 * This file is part of the UKHASNet (ukhas.net) maintained RFM69 library for
 * use with all UKHASnet nodes, including Arduino, AVR and ARM.
 *
 * Ported to Arduino 2014 James Coxon
 * Ported, tidied and hardware abstracted by Jon Sowman, 2015
 *
 * Copyright (C) 2014 Phil Crump
 * Copyright (C) 2015 Jon Sowman <jon@jonsowman.com>
 *
 * Based on RF22 Copyright (C) 2011 Mike McCauley
 * Ported to mbed by Karl Zweimueller
 *
 * Based on RFM69 LowPowerLabs (https://github.com/LowPowerLab/RFM69/)
 */

#include "ukhasnet-rfm69.h"
#include "spi_conf.h"

/**
 * User SPI setup function. Use this function to set up the SPI peripheral
 * on the microcontroller, such as to setup the IO, set the mode (0,0) for the
 * RFM69, and become a master.
 * @returns True on success, false on failure.
 */
bool rfm69_init(void)
{
    /* Set up the SPI IO as appropriate */
    SPI_DDR |= SPI_SS | SPI_MOSI | SPI_SCK;
    SPI_DDR &= ~(SPI_MISO);

    /* Set SS high */
    SPI_PORT |= SPI_SS;

    /* SPI should be mode (0,0), MSB first, double clock rate*/
    SPCR &= ~(_BV(CPOL) | _BV(CPHA) | _BV(DORD));
    SPSR |= _BV(SPI2X);

    /* Become master */
    SPCR |= _BV(MSTR);

    /* Finally, enable the SPI periph */
    SPCR |= _BV(SPE);

    /* Call the rf69 library function to init the radio */
    return rf69_init();
}

/**
 * User function to exchange a single byte over the SPI interface
 * @warn This does not handle SS, since higher level functions might want to do
 * burst read and writes
 * @param out The byte to be sent
 * @returns The byte received
 */
uint8_t spi_exchange_single(uint8_t out)
{
    SPDR = out;
    while(!(SPSR & (1<<SPIF)));
    return SPDR;
}

/**
 * User function to assert the slave select pin
 */
void spi_ss_assert(void)
{
    SPI_PORT &= ~(SPI_SS);
}

/**
 * User function to deassert the slave select pin
 */
void spi_ss_deassert(void)
{
    SPI_PORT |= (SPI_SS);
}

