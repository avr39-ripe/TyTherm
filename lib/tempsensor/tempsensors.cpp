/*
 * tempsensors.cpp
 *
 *  Created on: 6 марта 2016
 *      Author: shurik
 */

#include <tempsensors.h>

//TempSensos
TempSensors::TempSensors(uint16_t refresh)
{
	_refresh = refresh;
}

void TempSensors::start()
{
	_refreshTimer.initializeMs(_refresh, TimerDelegate(&TempSensors::_temp_start, this)).start(true);
}

void TempSensors::stop()
{
	_refreshTimer.stop();
}

void TempSensors::addSensor()
{
	auto newSensorData = new sensorData;
	_data.add(newSensorData);
}

//TempSensorsOW
TempSensorsOW::TempSensorsOW(OneWire &ds, uint16_t refresh)
:TempSensors(refresh)
{

	_ds = &ds;
}

void TempSensorsOW::addSensor(uint8_t* address)
{
	TempSensors::addSensor();
	_addresses.add(address);
}

void TempSensorsOW::_temp_start()
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

		_temp_readTimer.initializeMs(190, TimerDelegate(&TempSensorsOW::_temp_read, this)).start(false);
	}
}

void TempSensorsOW::_temp_read()
{
	for (uint8_t id=0; id < _addresses.count(); id++)
	{
		uint8_t _temp_data[12];
		_ds->reset();
		_ds->select(_addresses[id]);
	//	_ds->skip();
		_ds->write(0xBE); // Read Scratchpad

		for (uint8_t i = 0; i < 9; i++)
		{
			_temp_data[i] = _ds->read();
	//		Serial.printf("SP[%d]: %d", i, _temp_data[i]);
		}

		// Here we filter error, when NO DS18B20 actually connected
		// According to DS18B20 datasheet scratchpad[5] == 0xFF and scratchpad[7] == 0x10
		// At startup or when no GND connected scratchpad[0] == 0x50 and scratchpad[1] == 0x05
		// It is not often temperature is 85.000 degree so we FILTER OUT precise 85.000 readings
		if (_temp_data[5] != 0xFF || _temp_data[7] != 0x10)
		{
			Serial.printf("no DS18B20 device present at id: %d!\n", id);
			_data[id]->_statusFlag = (TempSensorStatus::DISCONNECTED | TempSensorStatus::INVALID);
			continue;
		}
		if (_temp_data[0] == 0x50 && _temp_data[1] == 0x05)
		{
			Serial.printf("DS18B20 id: %d invalid temperature\n", id);
			_data[id]->_statusFlag = TempSensorStatus::INVALID;
			continue;
		}
		if (OneWire::crc8(_temp_data, 8) != _temp_data[8])
		{
			Serial.printf("DS18B20 id: %d invalid crc!\n", id);
			_data[id]->_statusFlag = TempSensorStatus::INVALID;
			continue;
		}

		uint16_t tempRead = ((_temp_data[1] << 8) | _temp_data[0]); //using two's compliment

			if (tempRead & 0x8000)
				_data[id]->_temperature = 0 - ((float) ((tempRead ^ 0xffff) + 1) / 16.0);
			else
				_data[id]->_temperature = (float)(tempRead / 16.0);

			_data[id]->_statusFlag = 0; // current value of _temperature is GOOD, healthy
		}
	_temp_readTimer.stop();
}

