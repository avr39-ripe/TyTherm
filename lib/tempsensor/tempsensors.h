/*
 * tempsensors.h
 *
 *  Created on: 5 марта 2016
 *      Author: shurik
 */

#ifndef LIB_TEMPSENSOR_TEMPSENSORS_H_
#define LIB_TEMPSENSOR_TEMPSENSORS_H_
#include <SmingCore/SmingCore.h>
#include <Libraries/OneWire/OneWire.h>

namespace TempSensorStatus
{
	const uint8_t INVALID=1;
	const uint8_t DISCONNECTED=2;
}

struct sensorData
{
	float _temperature = 0;
	int16_t _calAdd = 0; //Additive calibration coefficient * 100
	int16_t _calMult = 0; //Multiplicative calibration coefficient * 100
	uint8_t _statusFlag = 0;
};

class TempSensors
{
public:
	TempSensors(uint16_t refresh = 4000);
	virtual ~TempSensors() {};
	void start();
	void stop();
	void addSensor();
	float getTemp(uint8_t sensorId);
	float getTemp() { return getTemp(0);}
	uint8_t isValid(uint8_t sensorId) { return (_data[sensorId]->_statusFlag & TempSensorStatus::INVALID) ? 0 : 1; };
	uint8_t isValid() { return isValid(0); }
	uint8_t isConnected(uint8_t sensorId) { return (_data[sensorId]->_statusFlag & TempSensorStatus::DISCONNECTED) ? 0 : 1;};
	uint8_t isConnected() { return isConnected(0); };
	void onHttpGet(HttpRequest &request, HttpResponse &response);
	void onHttpConfig(HttpRequest &request, HttpResponse &response);
	void _saveBinConfig();
	void _loadBinConfig();
protected:
	Vector<sensorData*> _data;
	uint16_t _refresh;
	Timer _refreshTimer;
	virtual void _temp_start() = 0;
};

class TempSensorsOW : public TempSensors
{
public:
	TempSensorsOW(OneWire &ds, uint16_t refresh = 4000);
	virtual ~TempSensorsOW() {};
	void addSensor();
	void addSensor(uint8_t* address);
	void addSensor(String address);
	void modifySensor(uint8_t sensorId, String address);
private:
	virtual void _temp_start();
	void _temp_read();
	void _hexStrToAddress(String addrStr, uint8_t* addrArray);
	Vector<uint8_t*> _addresses;
	OneWire *_ds;
	Timer _temp_readTimer;
};

class TempSensorsHttp : public TempSensors
{
public:
	TempSensorsHttp(uint16_t refresh = 4000);
	virtual ~TempSensorsHttp() {};
	void addSensor();
	void addSensor(String url);

private:
	virtual void _temp_start();
	void _temp_read(HttpClient& client, bool successful);
	void _getHttpTemp();
	HttpClient _httpClient;
	Vector<String> _addresses;
	uint8_t _currentSensorId;
	Timer _httpTimer;
};
#endif /* LIB_TEMPSENSOR_TEMPSENSORS_H_ */
