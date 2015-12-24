/*
 * tempsensor.cpp
 *
 *  Created on: 24 дек. 2015 г.
 *      Author: shurik
 */

#include <tempsensor.h>

TempSensor::TempSensor(OneWire &ds, uint16_t refresh, uint8_t retries)
{
	_ds = &ds;
	_refresh = refresh;
	_retries = retries;
	_temp_counter = 0;
	_temp_accum = 0;
	_temperature = 0;
}

void TempSensor::_temp_start()
{
	if (!_temp_readTimer.isStarted())
	{
		//set 10bit resolution
		_ds->reset();
		_ds->skip();
		_ds->write(0x4e); // write scratchpad cmd
		_ds->write(0xff); // write scratchpad 0
		_ds->write(0xff); // write scratchpad 1
		_ds->write(0b00111111); // write scratchpad config

		_ds->reset();
		_ds->skip();
		_ds->write(0x44); // start conversion

		_temp_readTimer.initializeMs(190, TimerDelegate(&TempSensor::_temp_read, this)).start(false);
	}
}

void TempSensor::start()
{
	_temp_startTimer.initializeMs(_refresh, TimerDelegate(&TempSensor::_temp_start, this)).start(true);
}

void TempSensor::stop()
{
	_temp_startTimer.stop();
}


void TempSensor::_temp_read()
{
	_ds->reset();
//	_ds->select(temp_sensors[n].addr);
	_ds->skip();
	_ds->write(0xBE); // Read Scratchpad

	for (uint8_t i = 0; i < 9; i++)
	{
		_temp_data[i] = _ds->read();
	}

	if (OneWire::crc8(_temp_data, 8) != _temp_data[8])
	{
//		Serial.println("DS18B20 temp crc error!");
		_temp_counter = 0;
		_temp_accum = 0;
		_temp_readTimer.stop();
		_temp_start();
		return;
	}
	uint16_t tempRead = ((_temp_data[1] << 8) | _temp_data[0]); //using two's compliment
	if (_temp_counter < _retries)
	{
		_temp_counter++;

		if (tempRead & 0x8000)
			_temp_accum += 0 - ((float) ((tempRead ^ 0xffff) + 1) / 16.0);
		else
			_temp_accum += (float)(tempRead / 16.0);

//		Serial.print("TA "); Serial.println(_temp_accum);
		_temp_readTimer.stop();
		_temp_start();
		return;
	}
	else
	{
		_temperature = _temp_accum / _retries;
		_temp_counter = 0;
		_temp_accum = 0;
//		Serial.print("MT "); Serial.println(_mode_curr_temp);
	}

//	Serial.print("_mode_curr_temp = "); Serial.println(_mode_curr_temp);
	_temp_readTimer.stop();
}

