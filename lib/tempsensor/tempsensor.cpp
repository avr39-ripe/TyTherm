/*
 * tempsensor.cpp
 *
 *  Created on: 24 дек. 2015 г.
 *      Author: shurik
 */

#include <tempsensor.h>

// TempSensor

TempSensor::TempSensor(uint16_t refresh)
{
	_refresh = refresh;
	_temperature = 0;
}

void TempSensor::start()
{
	_refreshTimer.initializeMs(_refresh, TimerDelegate(&TempSensor::_temp_start, this)).start(true);
}

void TempSensor::stop()
{
	_refreshTimer.stop();
}

void TempSensor::_temp_start()
{
	return;
}

// TempSensorOW

TempSensorOW::TempSensorOW(OneWire &ds, uint16_t refresh, uint8_t tries)
:TempSensor(refresh)
{

	_ds = &ds;
	_tries = tries;
	_temp_counter = 0;
	_temp_accum = 0;
}

void TempSensorOW::_temp_start()
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

		_temp_readTimer.initializeMs(190, TimerDelegate(&TempSensorOW::_temp_read, this)).start(false);
	}
}

//void TempSensorOW::start()
//{
//	_refreshTimer.initializeMs(_refresh, TimerDelegate(&TempSensorOW::_temp_start, this)).start(true);
//}
//
//void TempSensorOW::stop()
//{
//	_refreshTimer.stop();
//}


void TempSensorOW::_temp_read()
{
	uint8_t _temp_data[12];
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
	if (_temp_counter < _tries)
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
		_temperature = _temp_accum / _tries;
		_temp_counter = 0;
		_temp_accum = 0;
//		Serial.print("MT "); Serial.println(_mode_curr_temp);
	}

//	Serial.print("_mode_curr_temp = "); Serial.println(_mode_curr_temp);
	_temp_readTimer.stop();
}

// TempSensorHttp

TempSensorHttp::TempSensorHttp(String url, uint16_t refresh)
:TempSensor(refresh)
{
	_url = url;
}

void TempSensorHttp::_temp_start()
{
	if (_httpClient.isProcessing())
		return; // We need to wait while request processing was completed
	else
		_httpClient.downloadString(_url, HttpClientCompletedDelegate(&TempSensorHttp::_temp_read, this));

}

void TempSensorHttp::_temp_read(HttpClient& client, bool successful)
{
	if (successful)
	{
		String response = client.getResponseString();
		if (response.length() > 0)
		{
			StaticJsonBuffer<64> jsonBuffer;
			JsonObject& root = jsonBuffer.parseObject(response);
//			root.prettyPrintTo(Serial); //Uncomment it for debuging
			if (root["temperature"].success())
			{
				_temperature = root["temperature"];
			}
		}
	}
}
