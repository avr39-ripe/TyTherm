#ifndef INCLUDE_TEMPSENSOR_H_
#define INCLUDE_TEMPSENSOR_H_
#include <SmingCore/SmingCore.h>
#include <Libraries/OneWire/OneWire.h>

class TempSensor {
public:
	TempSensor(OneWire &ds, uint16_t refresh = 4000, uint8_t retries = 5);
	void start();
	void stop();
	float getTemp() { return _temperature; };
private:
	void _temp_start();
	void _temp_read();
	OneWire *_ds;
	uint16_t _refresh;
	uint8_t _retries;
	Timer _temp_startTimer;
	Timer _temp_readTimer;
	uint8_t _temp_data[12];
	float _temp_accum;
	uint8_t _temp_counter;
	float _temperature;
};


#endif /* INCLUDE_TEMPSENSOR_H_ */
