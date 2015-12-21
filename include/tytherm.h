#ifndef INCLUDE_TYTHERM_H_
#define INCLUDE_TYTHERM_H_
#include <Libraries/OneWire/OneWire.h>

//OneWire stuff
const uint8_t onewire_pin = 2;
extern OneWire ds;

extern unsigned long counter;

//Webserver
void startWebServer();

//STA disconnecter
const uint16_t StaConnectTimeout = 20; //15 sec to connect in STA mode
void StaConnectOk();
void StaConnectFail();

#endif /* INCLUDE_HEATCONTROL_H_ */
