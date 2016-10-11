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

float TempSensors::getTemp(uint8_t sensorId)
{
	return (_data[sensorId]->_temperature * (float)(_data[sensorId]->_calMult / 100.0)) + (float)(_data[sensorId]->_calAdd / 100.0);
};

void TempSensors::addSensor()
{
	auto newSensorData = new sensorData;
	_data.add(newSensorData);
}

void TempSensors::onHttpGet(HttpRequest &request, HttpResponse &response)
{
	if (request.getRequestMethod() == RequestMethod::GET)
	{
		DynamicJsonBuffer jsonBuffer;
		String buf;
		JsonObject& root = jsonBuffer.createObject();
		String queryParam = request.getQueryParameter("sensor", "-1");
		if (queryParam == "-1")
		{
			for (uint8_t id=0; id < _data.count(); id++)
			{
				JsonObject& data = root.createNestedObject((String)id);
				data["temperature"] = getTemp(id);
				data["statusFlag"] = _data[id]->_statusFlag;
			}
		}
		else
		{
			uint8_t id = request.getQueryParameter("sensor").toInt();
			if (id >= 0 && id < _data.count())
			{
				root["temperature"] = getTemp(id);
				root["statusFlag"] = _data[id]->_statusFlag;
			}
		}

		root.printTo(buf);

		response.setHeader("Access-Control-Allow-Origin", "*");
		response.setContentType(ContentType::JSON);
		response.sendString(buf);
	}
}

void TempSensors::onHttpConfig(HttpRequest &request, HttpResponse &response)
{
	if (request.getRequestMethod() == RequestMethod::POST)
		{
			if (request.getBody() == NULL)
			{
				debugf("NULL bodyBuf");
				return;
			}
			else
			{
				uint8_t needSave = false;
				DynamicJsonBuffer jsonBuffer;
				JsonObject& root = jsonBuffer.parseObject(request.getBody());
				root.prettyPrintTo(Serial); //Uncomment it for debuging
				String queryParam = request.getQueryParameter("sensor", "-1");
				if (queryParam != "-1")
				{
					uint8_t id = request.getQueryParameter("sensor").toInt();
					if (root["calAdd"].success()) // Settings
					{
						_data[id]->_calAdd = root["calAdd"];
						needSave = true;
					}
					if (root["calMult"].success()) // Settings
					{
						_data[id]->_calMult = root["calMult"];
						needSave = true;
					}
					if (needSave)
					{
						_saveBinConfig();
					}
				}
			}
		}
		else
		{
			DynamicJsonBuffer jsonBuffer;
			String buf;
			JsonObject& root = jsonBuffer.createObject();
			String queryParam = request.getQueryParameter("sensor", "-1");
			if (queryParam == "-1")
			{
				for (uint8_t id=0; id < _data.count(); id++)
				{
					JsonObject& data = root.createNestedObject((String)id);
					data["calAdd"] = _data[id]->_calAdd;
					data["calMult"] = _data[id]->_calMult;
				}
			}
			else
			{
				uint8_t id = request.getQueryParameter("sensor").toInt();
				if (id >= 0 && id < _data.count())
				{
					root["calAdd"] = _data[id]->_calAdd;
					root["calMult"] = _data[id]->_calMult;
				}
			}

			root.printTo(buf);

			response.setHeader("Access-Control-Allow-Origin", "*");
			response.setContentType(ContentType::JSON);
			response.sendString(buf);
		}
}

void TempSensors::_saveBinConfig()
{
	Serial.printf("Try to save bin cfg..\n");
	file_t file = fileOpen("tmpsensors", eFO_CreateIfNotExist | eFO_WriteOnly);
	for (uint8_t id=0; id < _data.count(); id++)
	{
		fileWrite(file, &_data[id]->_calAdd, sizeof(_data[id]->_calAdd));
		fileWrite(file, &_data[id]->_calMult, sizeof(_data[id]->_calMult));
	}
	fileClose(file);
}

void TempSensors::_loadBinConfig()
{
	Serial.printf("Try to load bin cfg..\n");
	if (fileExist("tmpsensors"))
	{
		Serial.printf("Will load bin cfg..\n");
		file_t file = fileOpen("tmpsensors", eFO_ReadOnly);
		fileSeek(file, 0, eSO_FileStart);
		for (uint8_t id=0; id < _data.count(); id++)
		{
			fileRead(file, &_data[id]->_calAdd, sizeof(_data[id]->_calAdd));
			fileRead(file, &_data[id]->_calMult, sizeof(_data[id]->_calMult));
		}
		fileClose(file);
	}
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
	uint8_t* sensorAddr = new uint8_t[8]; //Reserve memory for class-copy of addres
	if (address == nullptr)
	{ return; }
	else
	{ os_memcpy(sensorAddr, address, 8); }
	_addresses.add(sensorAddr);
}

void TempSensorsOW::addSensor(String address)
{
	TempSensors::addSensor();
	uint8_t* sensorAddr = new uint8_t[8]; //Reserve memory for class-copy of addres
	os_memset(sensorAddr, 0, 8);
	_hexStrToAddress(address, sensorAddr);

	_addresses.add(sensorAddr);
}
void TempSensorsOW::modifySensor(uint8_t sensorId, String address)
{
	uint8_t* sensorAddr = new uint8_t[8]; //Reserve memory for class-copy of addres
	os_memset(sensorAddr, 0, 8);
	_hexStrToAddress(address, sensorAddr);

	os_memcpy(_addresses[sensorId],sensorAddr, 8);
	delete[] sensorAddr;
}

void TempSensorsOW::_hexStrToAddress(String addrStr, uint8_t* addrArray)
{
	auto strLength = addrStr.length();
	if (strLength != 16 || addrArray == nullptr) //address must be 8 byte value in hex format with 2 digits per byte so length must be exactly 16
	{
		return;
	}
	else
	{
		for (uint8_t index = 0; index < strLength; index++)
		{
			uint8_t c = addrStr[index];
			uint8_t value = 0;
			if(c >= '0' && c <= '9')
			{
				value = (c - '0');
			}
			else if (c >= 'A' && c <= 'F')
			{
				value = (10 + (c - 'A'));
			}
			else if (c >= 'a' && c <= 'f')
			{
				 value = (10 + (c - 'a'));
			}
			else
			{
				return;
			}
			addrArray[(index/2)] += value << (((index + 1) % 2) * 4);
		}
		Serial.printf("OWADDR: ");
		for (uint8_t i = 0; i<8; i++)
		{
			Serial.printf("%02X", addrArray[i]);
		}
		Serial.println();
	}
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
//			Serial.printf("SP[%d]: %d", i, _temp_data[i]);
		}

		// Here we filter error, when NO DS18B20 actually connected
		// According to DS18B20 datasheet scratchpad[5] == 0xFF and scratchpad[7] == 0x10
		// At startup or when no GND connected scratchpad[0] == 0x50 and scratchpad[1] == 0x05
		// It is not often temperature is 85.000 degree so we FILTER OUT precise 85.000 readings
		if (_temp_data[5] == 0xFF && (_temp_data[7] == 0x10 || _temp_data[7] == 0x80))
		{
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
		else
		{
			Serial.printf("no DS18B20 device present at id: %d!\n", id);
			_data[id]->_statusFlag = (TempSensorStatus::DISCONNECTED | TempSensorStatus::INVALID);
			continue;
		}
	}
	_temp_readTimer.stop(); //stop temp_readTimer after ALL sensors data red, not after first successful!!!
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

void TempSensorsHttp::_getHttpTemp()
{
	if (_httpClient.isProcessing())
	{
		return; // We need to wait while request processing was completed
	}
	else
	{
		_httpClient.reset();
		_httpClient.downloadString(_addresses[_currentSensorId], HttpClientCompletedDelegate(&TempSensorsHttp::_temp_read, this));
	}

}
void TempSensorsHttp::_temp_start()
{
	if (_currentSensorId == 0)
	{
		_getHttpTemp();
	}
	else
	{
		Serial.printf("Last sequesnce not complete, will wait another period!\n");
		_refreshTimer.start(true);
	}
}

void TempSensorsHttp::_temp_read(HttpClient& client, bool successful)
{
//	Serial.println("temp-read");
	if (successful)
	{
//	Serial.println("tr-succes");
		String response = client.getResponseString();
		if (response.length() > 0)
		{
//		Serial.println("res>0");
			StaticJsonBuffer<200> jsonBuffer;
			JsonObject& root = jsonBuffer.parseObject(response);
//			root.prettyPrintTo(Serial); //Uncomment it for debuging
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
		_currentSensorId++;
		_httpTimer.initializeMs(100, TimerDelegate(&TempSensorsHttp::_getHttpTemp, this)).start(false);
//		_getHttpTemp(_currentSensorId++);
	}
	else
	{
		Serial.printf("Last sensor! Wait for timer event!\n");
		_currentSensorId = 0;
	}
}
