/**
 * spi_conf.h
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

#ifndef __SPI_CONF_H__
#define __SPI_CONF_H__

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>

/* SPI pins and ports */
#define SPI_DDR     DDRB
#define SPI_PORT    PORTB
#define SPI_SS      _BV(2)
#define SPI_MOSI    _BV(3)
#define SPI_MISO    _BV(4)
#define SPI_SCK     _BV(5)

#endif /* __SPI_CONF_H__ */
