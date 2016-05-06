#include <app.h>
#include <user_config.h>
#include <tytherm.h>

OneWire ds(onewire_pin);
TempSensors* tempSensor;

AppClass App;

void init()
{
        App.init();
        App.start();
}
