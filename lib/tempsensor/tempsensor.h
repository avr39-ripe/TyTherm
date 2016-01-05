#ifndef INCLUDE_TEMPSENSOR_H_
#define INCLUDE_TEMPSENSOR_H_
#include <SmingCore/SmingCore.h>
#include <Libraries/OneWire/OneWire.h>

class TempSensor
{
public:
	TempSensor(uint16_t refresh = 4000);
	virtual ~TempSensor() {};
	void start();
	void stop();
	float getTemp() { return _temperature; };
protected:
	float _temperature;
	uint16_t _refresh;
	Timer _refreshTimer;
	virtual void _temp_start();
};

class TempSensorOW : public TempSensor
{
public:
	TempSensorOW(OneWire &ds, uint16_t refresh = 4000, uint8_t tries = 5);
	virtual ~TempSensorOW() {};
private:
	virtual void _temp_start();
	void _temp_read();
	OneWire *_ds;
	uint8_t _tries;
	Timer _temp_readTimer;
//	uint8_t _temp_data[12];
	float _temp_accum;
	uint8_t _temp_counter;
};

class TempSensorHttp : public TempSensor
{
public:
	TempSensorHttp(String url, uint16_t refresh = 4000);
	virtual ~TempSensorHttp() {};
private:
	virtual void _temp_start();
	void _temp_read(HttpClient& client, bool successful);
	HttpClient _httpClient;
	String _url;
};


#endif /* INCLUDE_TEMPSENSOR_H_ */
