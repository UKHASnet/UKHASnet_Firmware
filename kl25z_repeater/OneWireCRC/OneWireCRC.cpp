/*
* OneWireCRC. This is a port to mbed of Jim Studt's Adruino One Wire
* library. Please see additional copyrights below this one, including
* references to other copyrights.
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
/*
Copyright (c) 2007, Jim Studt

Updated to work with arduino-0008 and to include skip() as of
2007/07/06. --RJL20

Modified to calculate the 8-bit CRC directly, avoiding the need for
the 256-byte lookup table to be loaded in RAM.  Tested in arduino-0010
-- Tom Pollard, Jan 23, 2008

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Much of the code was inspired by Derek Yerger's code, though I don't
think much of that remains.  In any event that was..
    (copyleft) 2006 by Derek Yerger - Free to distribute freely.

The CRC code was excerpted and inspired by the Dallas Semiconductor 
sample code bearing this copyright.
//---------------------------------------------------------------------------
// Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
// OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// Except as contained in this notice, the name of Dallas Semiconductor
// shall not be used except as stated in the Dallas Semiconductor
// Branding Policy.
//--------------------------------------------------------------------------
*/

#include "OneWireCRC.h"
#include "OneWireDefs.h"

// recommended data sheet timings in micro seconds
const int standardT[] = {6, 64, 60, 10, 9, 55, 0, 480, 70, 410};
const int overdriveT[] = {1.5, 7.5, 7.5, 2.5, 0.75, 7, 2.5, 70, 8.5, 40};

OneWireCRC::OneWireCRC(PinName oneWire, eSpeed speed) : oneWirePort(oneWire)
{
    if (STANDARD == speed) timing = standardT;   
    else  timing = overdriveT;  // overdrive
    
    resetSearch();    // reset address search state
}

// Generate a 1-wire reset, return 1 if no presence detect was found,
// return 0 otherwise.
// (NOTE: does not handle alarm presence from DS2404/DS1994)
int OneWireCRC::reset() 
{
    
    BYTE result = 0;    // sample presence pulse result
        
    wait_us(timing[6]);
    oneWirePort.output();
    oneWirePort = 0;
    wait_us(timing[7]);
    oneWirePort.input();
    wait_us(timing[8]);
    result = !(oneWirePort & 0x01);
    wait_us(timing[9]);
    
    return result;
}

//
// Write a bit. Port and bit is used to cut lookup time and provide
// more certain timing.
//
void OneWireCRC::writeBit(int bit)
{
    bit = bit & 0x01;
    
    if (bit)
    {
        // Write '1' bit
        oneWirePort.output();
        oneWirePort = 0;
        wait_us(timing[0]);
        oneWirePort.input();
        wait_us(timing[1]);
    }
    else
    {
        // Write '0' bit
        oneWirePort.output();
        oneWirePort = 0;
        wait_us(timing[2]);
        oneWirePort.input();
        wait_us(timing[3]);
    }
}

//
// Read a bit. Port and bit is used to cut lookup time and provide
// more certain timing.
//
int OneWireCRC::readBit() 
{
    BYTE result;
    
    oneWirePort.output();
    oneWirePort = 0;
    wait_us(timing[0]);
    oneWirePort.input();
    wait_us(timing[4]);
    result = oneWirePort & 0x01;
    wait_us(timing[5]);
       
    return result;
}

//
// Write a byte. The writing code uses the active drivers to raise the
// pin high, if you need power after the write (e.g. DS18S20 in
// parasite power mode) then set 'power' to 1, otherwise the pin will
// go tri-state at the end of the write to avoid heating in a short or
// other mishap.
//
void OneWireCRC::writeByte(int data) 
{
    // Loop to write each bit in the byte, LS-bit first
    for (int loop = 0; loop < 8; loop++)
    {
        writeBit(data & 0x01);
        
        // shift the data byte for the next bit
        data >>= 1;
    }
}

//
// Read a byte
//
int OneWireCRC::readByte() 
{
    int result = 0;
    
    for (int loop = 0; loop < 8; loop++)
    {
        // shift the result to get it ready for the next bit
        result >>= 1;
        
        // if result is one, then set MS bit
        if (readBit()) result |= 0x80;
    }
    
    return result;
}

int OneWireCRC::touchByte(int data)
{
    int result = 0;
    
    for (int loop = 0; loop < 8; loop++)
    {
        // shift the result to get it ready for the next bit
        result >>= 1;
        
        // If sending a '1' then read a bit else write a '0'
        if (data & 0x01)
        {
            if (readBit()) result |= 0x80;
        }
        else writeBit(0);
        
        // shift the data byte for the next bit
        data >>= 1;
    }
    
    return result;
}

void OneWireCRC::block(BYTE* data, int data_len)
{
    for (int loop = 0; loop < data_len; loop++)
    {
        data[loop] = touchByte(data[loop]);
    }
}

int OneWireCRC::overdriveSkip(BYTE* data, int data_len)
{
    // set the speed to 'standard'
    timing = standardT;
    
    // reset all devices
    if (reset()) return 0;    // if no devices found
    
    // overdrive skip command
    writeByte(OVERDRIVE_SKIP);
    
    // set the speed to 'overdrive'
    timing = overdriveT;
    
    // do a 1-Wire reset in 'overdrive' and return presence result
    return reset();
}

//
// Do a ROM select
//
void OneWireCRC::matchROM(BYTE rom[8])
{
    writeByte(MATCH_ROM);         // Choose ROM

    for(int i = 0; i < 8; i++) writeByte(rom[i]);
}

//
// Do a ROM skip
//
void OneWireCRC::skipROM()
{
    writeByte(SKIP_ROM);         // Skip ROM
}

//
// You need to use this function to start a search again from the beginning.
// You do not need to do it for the first search, though you could.
//
void OneWireCRC::resetSearch()
{
    searchJunction = -1;
    searchExhausted = false;
    for (int i = 0; i < 8; i++) 
    {
        address[i] = 0;
    }
}

//
// Perform a search. If this function returns a '1' then it has
// enumerated the next device and you may retrieve the ROM from the
// OneWire::address variable. If there are no devices, no further
// devices, or something horrible happens in the middle of the
// enumeration then a 0 is returned.  If a new device is found then
// its address is copied to newAddr.  Use OneWire::reset_search() to
// start over.
// 
BYTE OneWireCRC::search(BYTE* newAddr)
{
    BYTE i;
    int lastJunction = -1;
    BYTE done = 1;
    
    if (searchExhausted) return 0;
    
    if (!reset()) return 0;

    writeByte(SEARCH_ROM);
    
    for(i = 0; i < 64; i++) 
    {
        BYTE a = readBit( );
        BYTE nota = readBit( );
        BYTE ibyte = i/8;
        BYTE ibit = 1 << (i & 7);
    
        // I don't think this should happen, this means nothing responded, but maybe if
        // something vanishes during the search it will come up.
        if (a && nota) return 0;  
        
        if (!a && !nota)
        {
            if (i == searchJunction) 
            {
                // this is our time to decide differently, we went zero last time, go one.
                a = 1;
                searchJunction = lastJunction;
            } 
            else if (i < searchJunction) 
            {
                // take whatever we took last time, look in address
                if (address[ibyte] & ibit) a = 1;
                else 
                {
                    // Only 0s count as pending junctions, we've already exhasuted the 0 side of 1s
                    a = 0;
                    done = 0;
                    lastJunction = i;
                }
            } 
            else 
            {
                // we are blazing new tree, take the 0
                a = 0;
                searchJunction = i;
                done = 0;
            }
            lastJunction = i;
        }
        
        if (a) address[ibyte] |= ibit;
        else address[ibyte] &= ~ibit;
    
        writeBit(a);
    }
    
    if (done) searchExhausted = true;
    
    for (i = 0; i < 8; i++) newAddr[i] = address[i];
    
    return 1;  
}

// The 1-Wire CRC scheme is described in Maxim Application Note 27:
// "Understanding and Using Cyclic Redundancy Checks with Maxim iButton Products"
//

#if ONEWIRE_CRC8_TABLE
// This table comes from Dallas sample code where it is freely reusable, 
// though Copyright (C) 2000 Dallas Semiconductor Corporation
static BYTE dscrc_table[] = 
{
      0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
    157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
     35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
    190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
     70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
    219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
    101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
    248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
    140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
     17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
    175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
     50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
    202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
     87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
    233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
    116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};

//
// Compute a Dallas Semiconductor 8 bit CRC. These show up in the ROM
// and the registers.  (note: this might better be done without the
// table, it would probably be smaller and certainly fast enough
// compared to all those delayMicrosecond() calls.  But I got
// confused, so I use this table from the examples.)  
//
BYTE OneWireCRC::crc8(BYTE* addr, BYTE len)
{
    BYTE i;
    BYTE crc = 0;
    
    for (i = 0; i < len; i++)
    {
        crc  = dscrc_table[crc ^ addr[i] ];
    }
    
    return crc;
}
#else
//
// Compute a Dallas Semiconductor 8 bit CRC directly. 
//
BYTE OneWireCRC::crc8(BYTE* addr, BYTE len)
{
    BYTE i, j;
    BYTE crc = 0;
    
    for (i = 0; i < len; i++) 
    {
        BYTE inbyte = addr[i];
        for (j = 0; j < 8; j++) 
        {
            BYTE mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    
    return crc;
}
#endif

static short oddparity[16] = { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 };

//
// Compute a Dallas Semiconductor 16 bit CRC. I have never seen one of
// these, but here it is.
//
unsigned short OneWireCRC::crc16(unsigned short* data, unsigned short len)
{
    unsigned short i;
    unsigned short crc = 0;
    
    for ( i = 0; i < len; i++) 
    {
        unsigned short cdata = data[len];
    
        cdata = (cdata ^ (crc & 0xff)) & 0xff;
        crc >>= 8;
    
        if (oddparity[cdata & 0xf] ^ oddparity[cdata >> 4]) crc ^= 0xc001;
    
        cdata <<= 6;
        crc ^= cdata;
        cdata <<= 1;
        crc ^= cdata;
    }
    
    return crc;
}

