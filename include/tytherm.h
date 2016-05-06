/*
 * tytherm.h
 *
 *  Created on: 30 марта 2016
 *      Author: shurik
 */

#ifndef INCLUDE_TYTHERM_H_
#define INCLUDE_TYTHERM_H_
#include <SmingCore/SmingCore.h>
#include <Libraries/OneWire/OneWire.h>
#include <tempsensors.h>

//OneWire stuff
const uint8_t onewire_pin = 2;
extern OneWire ds;
extern TempSensors* tempSensor;

#endif /* INCLUDE_DRAGONMASTER_H_ */
