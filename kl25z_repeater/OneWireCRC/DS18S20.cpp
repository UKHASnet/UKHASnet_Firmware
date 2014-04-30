/*
* DS18S20. Maxim DS18S20 One-Wire Thermometer. 
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

#include "DS18S20.h"


DS18S20::DS18S20(bool crcOn, bool useAddr, bool parasitic, PinName pin) : 
    OneWireThermometer(crcOn, useAddr, parasitic, pin, DS18S20_ID)
{
}

float DS18S20::calculateTemperature(BYTE* data)
{    
    // DS18S20 basic resolution is always 9 bits, which can be enhanced as follows
    bool signBit = false;
    if (data[TEMPERATURE_MSB] & 0x80) signBit = true;
        
    int read_temp = (data[TEMPERATURE_MSB] << 8) + data[TEMPERATURE_LSB];
    if (signBit)
    {
        read_temp = (read_temp ^ 0xFFFF) + 1;    // two's complement
        read_temp *= -1;
    }
                 
    float readTemp = (float)read_temp/2 ;            // divide by 2
               
    // convert to real temperature
    float tempCount = float(data[COUNT_PER_DEG_BYTE] - data[COUNT_REMAIN_BYTE])/(float)data[COUNT_PER_DEG_BYTE];        
    float realTemp = (readTemp - 0.25) + tempCount;        
    
    return realTemp;
}