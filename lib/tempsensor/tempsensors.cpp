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

void TempSensorsOW::addSensor()
{
	TempSensors::addSensor();
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
	for (uint8_t id=0; id < _data.count(); id++)
	{
		uint8_t _temp_data[12];
		_ds->reset();
		if (_addresses.count() > 0)
		{
			_ds->select(_addresses[id]);
		}
		else
		{
			_ds->skip();
		}
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
			Serial.printf("ID: %d - ", id); Serial.println(_data[id]->_temperature);
		}
	_temp_readTimer.stop();
}

void TempSensorsOW::onHttpGet(HttpRequest &request, HttpResponse &response)
{
	if (request.getRequestMethod() == RequestMethod::GET)
	{
		DynamicJsonBuffer jsonBuffer;
		String buf;
		JsonObject& root = jsonBuffer.createObject();
		String queryParam = request.getQueryParameter("sensor", "-1");
//		Serial.printf("QueryParameter %s\n", queryParam.c_str());
		if (queryParam == "-1")
		{
//			Serial.printf("MultiSensor\n");
			for (uint8_t id=0; id < _data.count(); id++)
			{
				JsonObject& data = root.createNestedObject((String)id);
				data["temperature"] = _data[id]->_temperature;
				data["statusFlag"] = _data[id]->_statusFlag;
			}
		}
		else
		{
//			Serial.printf("SingleSensor\n");
			uint8_t id = request.getQueryParameter("sensor").toInt();
			if (id >= 0 && id < _data.count())
			{
				root["temperature"] = _data[id]->_temperature;
				root["statusFlag"] = _data[id]->_statusFlag;
			}
//			else
//			{
//				Serial.printf("Out of Range!\n");
//			}
		}

		root.printTo(buf);

		response.setHeader("Access-Control-Allow-Origin", "*");
		response.setContentType(ContentType::JSON);
		response.sendString(buf);
	}
}

//TempSensorsHttp
TempSensorsHttp::TempSensorsHttp(uint16_t refresh)
:TempSensors(refresh)
{
	_currentSensorId = 0;
}

void TempSensorsHttp::addSensor()
{
	TempSensors::addSensor();
}

void TempSensorsHttp::addSensor(String url)
{
	TempSensors::addSensor();
	_addresses.add(url);
}
//void TempSensorsHttp::start()
//{
//	_refreshTimer.initializeMs(_refresh, TimerDelegate(&TempSensors::_temp_start, this)).start(false);
//}
void TempSensorsHttp::_getHttpTemp(uint8_t sensorId)
{
	if (_httpClient->isProcessing())
	{
		return; // We need to wait while request processing was completed
	}
	else
	{
		if (_httpClient !=nullptr)
		{
			delete _httpClient;
		}
		else
		{
		_httpClient = new HttpClient();
		}
		_httpClient->downloadString(_addresses[_currentSensorId], HttpClientCompletedDelegate(&TempSensorsHttp::_temp_read, this));
	}

}
void TempSensorsHttp::_temp_start()
{
	if (_currentSensorId == 0)
	{
		_getHttpTemp(_currentSensorId);
	}
	else
	{
		Serial.printf("Last sequesnce not complete, will wait another period!\n");
		_refreshTimer.start(true);
	}
}

void TempSensorsHttp::_temp_read(HttpClient& client, bool successful)
{
	Serial.println("temp-read");
	if (successful)
	{
	Serial.println("tr-succes");
		String response = client.getResponseString();
		if (response.length() > 0)
		{
		Serial.println("res>0");
			StaticJsonBuffer<200> jsonBuffer;
			JsonObject& root = jsonBuffer.parseObject(response);
			root.prettyPrintTo(Serial); //Uncomment it for debuging
			if (root["temperature"].success())
			{
				_data[_currentSensorId]->_temperature = root["temperature"];
				_data[_currentSensorId]->_statusFlag = root["statusFlag"];
			}
			Serial.printf("ID: %d - ", _currentSensorId); Serial.println(_data[_currentSensorId]->_temperature);
		}
	}
	else
	{
		_data[_currentSensorId]->_statusFlag = (TempSensorStatus::DISCONNECTED | TempSensorStatus::INVALID);
		Serial.printf("NET PROBLEM unsucces request\n");
	}
	if (_currentSensorId < _data.count()-1)
	{
		Serial.printf("Read next sensor: %d\n", _currentSensorId + 1);
		_getHttpTemp(_currentSensorId++);
	}
	else
	{
		Serial.printf("Last sensor! Wait for timer event!\n");
		_currentSensorId = 0;
	}
}

void TempSensorsHttp::onHttpGet(HttpRequest &request, HttpResponse &response)
{
	if (request.getRequestMethod() == RequestMethod::GET)
	{
		DynamicJsonBuffer jsonBuffer;
		String buf;
		JsonObject& root = jsonBuffer.createObject();
		String queryParam = request.getQueryParameter("sensor", "-1");
//		Serial.printf("QueryParameter %s\n", queryParam.c_str());
		if (queryParam == "-1")
		{
//			Serial.printf("MultiSensor\n");
			for (uint8_t id=0; id < _data.count(); id++)
			{
				JsonObject& data = root.createNestedObject((String)id);
				data["temperature"] = _data[id]->_temperature;
				data["statusFlag"] = _data[id]->_statusFlag;
			}
		}
		else
		{
//			Serial.printf("SingleSensor\n");
			uint8_t id = request.getQueryParameter("sensor").toInt();
			if (id >= 0 && id < _data.count())
			{
				root["temperature"] = _data[id]->_temperature;
				root["statusFlag"] = _data[id]->_statusFlag;
			}
//			else
//			{
//				Serial.printf("Out of Range!\n");
//			}
		}

		root.printTo(buf);

		response.setHeader("Access-Control-Allow-Origin", "*");
		response.setContentType(ContentType::JSON);
		response.sendString(buf);
	}
}
