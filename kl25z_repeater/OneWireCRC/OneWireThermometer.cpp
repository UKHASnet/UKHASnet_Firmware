/*
* OneWireThermometer. Base class for Maxim One-Wire Thermometers. 
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

#include "OneWireThermometer.h"
#include "OneWireDefs.h"

// constructor specifies standard speed for the 1-Wire comms
OneWireThermometer::OneWireThermometer(bool crcOn, bool useAddr, bool parasitic, PinName pin, int device_id) :
    useCRC(crcOn), useAddress(useAddr), useParasiticPower(parasitic), 
    oneWire(pin, STANDARD), deviceId(device_id), resolution(twelveBit)
{
    // NOTE: the power-up resolution of a DS18B20 is 12 bits. The DS18S20's resolution is always
    // 9 bits + enhancement, but we treat the DS18S20 as fixed to 12 bits for calculating the
    // conversion time Tconv.
}

bool OneWireThermometer::initialize()
{
    // get the device address for use in selectROM() when reading the temperature
    // - not really needed except for device validation if using skipROM()
    if (useAddress)
    {
        oneWire.resetSearch();    
        if (!oneWire.search(address))   // search for 1-wire device address
        {            
            wait(2);
            return false;
        }

        for (int i = 0; i < ADDRESS_SIZE; i++) 
        {
        }
        
        if (OneWireCRC::crc8(address, ADDRESS_CRC_BYTE) != address[ADDRESS_CRC_BYTE])   // check address CRC is valid
        {
            wait(2);
            return false;
        }

        if (address[0] != deviceId)
        {                    
            
            wait(2);
            return false;   
        }
    }
    
    return true;
}

// NOTE ON USING SKIP ROM: ok to use before a Convert command to get all
// devices on the bus to do simultaneous temperature conversions. BUT can 
// only use before a Read Scratchpad command if there is only one device on the
// bus. For purpose of this library it is assumed there is only one device
// on the bus.
void OneWireThermometer::resetAndAddress()
{
    oneWire.reset();                // reset device 
    if (useAddress)
    {
        oneWire.matchROM(address);  // select which device to talk to
    }
    else
    {
        oneWire.skipROM();          // broadcast
    }
}

bool OneWireThermometer::readAndValidateData(BYTE* data)
{
    bool dataOk = true;
    
    resetAndAddress();
    oneWire.writeByte(READSCRATCH);    // read Scratchpad

    for (int i = 0; i < THERMOM_SCRATCHPAD_SIZE; i++) 
    {               
        // we need all bytes which includes CRC check byte
        data[i] = oneWire.readByte();
    }

    // Check CRC is valid if you want to
    if (useCRC && !(OneWireCRC::crc8(data, THERMOM_CRC_BYTE) == data[THERMOM_CRC_BYTE]))  
    {  
        // CRC failed
        dataOk = false;
    }
    
    return dataOk;
}

float OneWireThermometer::readTemperature()
{
    BYTE data[THERMOM_SCRATCHPAD_SIZE];
    float realTemp = -999;

    resetAndAddress();
    oneWire.writeByte(CONVERT);     // issue Convert command
    
    if (useParasiticPower)
    {
        // wait while converting - Tconv (according to resolution of reading)
        wait_ms(CONVERSION_TIME[resolution]);
    }
    else
    {
        // TODO
        // after the Convert command, the device should respond by transmitting 0
        // while the temperature conversion is in progress and 1 when the conversion is done
        // - as were are not checking this (TODO), we use Tconv, as we would do for
        // parasitic power 
        wait_ms(CONVERSION_TIME[resolution]);
    }

    if (readAndValidateData(data))    // issue Read Scratchpad commmand and get data
    {
        realTemp = calculateTemperature(data);
    }
    
    return realTemp; 
}