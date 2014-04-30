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

#ifndef SNATCH59_DS18B20_H
#define SNATCH59_DS18B20_H

#include "OneWireThermometer.h"
#include "OneWireDefs.h"

class DS18B20 : public OneWireThermometer
{
public:
    DS18B20(bool crcOn, bool useAddr, bool parasitic, PinName pin);
    
    virtual void setResolution(eResolution resln);
    
protected:
    virtual float calculateTemperature(BYTE* data);
};


#endif