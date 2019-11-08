/*
 * tytherm.h
 *
 *  Created on: 30 марта 2016
 *      Author: shurik
 */
#pragma once
#include <SmingCore.h>
#include <Libraries/OneWire/OneWire.h>
#include <tempsensors.h>

//OneWire stuff
const uint8_t onewire_pin = 2;
extern OneWire ds;
extern TempSensors* tempSensor;
