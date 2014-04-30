/*
* OneWireCRC. This is a port to mbed of Jim Studt's Adruino One Wire
* library.
*
* Copyright (C) <2009> Petras Saduikis <petras@petras.co.uk>
*
* This file is part of OneWireCRC.
*
* OneWireCRC is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
* 
* OneWireCRC is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with OneWireCRC.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SNATCH59_ONEWIREDEFS_H
#define SNATCH59_ONEWIREDEFS_H

// device ids
#define DS18B20_ID        0x28
#define DS18S20_ID        0x10

#define ALARM_CONFIG_SIZE 3
#define THERMOM_SCRATCHPAD_SIZE    9
#define THERMOM_CRC_BYTE  8
#define ADDRESS_SIZE      8
#define ADDRESS_CRC_BYTE  7

// One Wire command codes
#define OVERDRIVE_SKIP    0x3C
// ROM commands
#define SEARCH_ROM        0xF0
#define READ_ROM          0x33
#define MATCH_ROM         0x55
#define SKIP_ROM          0xCC
#define ALARM_SEARCH      0xEC
// Functions Commnds
#define CONVERT           0x44
#define WRITESCRATCH      0x4E
#define READSCRATCH       0xBE
#define COPYSCRATCH       0x48
#define RECALLE2          0xB8
#define READPOWERSUPPLY   0xB4

// temperature read resolutions
enum eResolution {nineBit = 0, tenBit, elevenBit, twelveBit};
const int CONVERSION_TIME[] = {94, 188, 375, 750};    // milli-seconds

// DS18B20/DS18S20 related
#define TEMPERATURE_LSB    0
#define TEMPERATURE_MSB    1
#define HIGH_ALARM_BYTE    2
#define LOW_ALARM_BYTE     3
#define CONFIG_REG_BYTE    4
#define CONFIG_READ_END    5
#define COUNT_REMAIN_BYTE  6
#define COUNT_PER_DEG_BYTE 7

#endif
