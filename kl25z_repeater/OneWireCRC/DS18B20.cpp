/*
* DS18B20. Maxim DS18B20 One-Wire Thermometer. 
* Uses the OneWireCRC library.
*
* Copyright (C) <2010> Petras Saduikis <petras@petras.co.uk>
*
* This file is part of OneWireThermometer.
*
* OneWireThermometer is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
* 
* OneWireThermometer is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with OneWireThermometer.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "DS18B20.h"


DS18B20::DS18B20(bool crcOn, bool useAddr, bool parasitic, PinName pin) : 
    OneWireThermometer(crcOn, useAddr, parasitic, pin, DS18B20_ID)
{
}

void DS18B20::setResolution(eResolution resln)
{
    // as the write to the configuration register involves a write to the
    // high and low alarm bytes, need to read these registers first
    // and copy them back on the write
    
    BYTE read_data[THERMOM_SCRATCHPAD_SIZE];
    BYTE write_data[ALARM_CONFIG_SIZE];
    
    if (readAndValidateData(read_data))
    {
        // copy alarm and config data to write data
        for (int k = 2; k < 5; k++)
        {
            write_data[k - 2] = read_data[k];
        }
        int config = write_data[2];
        config &= 0x9F;
        config ^= (resln << 5);
        write_data[2] = config;
        
        resetAndAddress();
        oneWire.writeByte(WRITESCRATCH);
        for (int k = 0; k < 3; k++)
        {
            oneWire.writeByte(write_data[k]);
        }
        
        // remember it so we can use the correct delay in reading the temperature
        // for parasitic power
        resolution = resln; 
    }
}

float DS18B20::calculateTemperature(BYTE* data)
{
    bool signBit = false;
    if (data[TEMPERATURE_MSB] & 0x80) signBit = true;
        
    int read_temp = (data[TEMPERATURE_MSB] << 8) + data[TEMPERATURE_LSB];
    if (signBit)
    {
        read_temp = (read_temp ^ 0xFFFF) + 1;    // two's complement
        read_temp *= -1;
    }
    
    int resolution = (data[CONFIG_REG_BYTE] & 0x60) >> 5; // mask off bits 6,5 and move to 1,0
    switch (resolution)
    {
        case nineBit:    // 0.5 deg C increments
            read_temp &= 0xFFF8;                // bits 2,1,0 are undefined
            break;
        case tenBit:     // 0.25 deg C increments
            read_temp &= 0xFFFC;                // bits 1,0 are undefined
            break;
        case elevenBit:  // 0.125 deg C increments
            read_temp &= 0xFFFE;                // bit 0 is undefined
            break;
        case twelveBit:  // 0.0625 deg C increments
            break;
    }
    float realTemp = (float)read_temp/16 ;
               
    return realTemp;
}