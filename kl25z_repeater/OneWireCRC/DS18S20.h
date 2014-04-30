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

#ifndef SNATCH59_DS18S20_H
#define SNATCH59_DS18S20_H

#include "OneWireThermometer.h"

class DS18S20 : public OneWireThermometer
{
public:
    DS18S20(bool crcOn, bool useAddr, bool parasitic, PinName pin);
    
     virtual void setResolution(eResolution resln) {  };    // do nothing
    
protected:
    virtual float calculateTemperature(BYTE* data);
};

#endif