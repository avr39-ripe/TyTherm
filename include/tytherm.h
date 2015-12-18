#ifndef INCLUDE_TYTHERM_H_
#define INCLUDE_TYTHERM_H_
#include <Libraries/OneWire/OneWire.h>

//OneWire stuff
const uint8_t onewire_pin = 2;
extern OneWire ds;

extern unsigned long counter;

//Webserver
void startWebServer();

#endif /* INCLUDE_HEATCONTROL_H_ */
