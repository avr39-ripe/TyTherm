#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <Libraries/OneWire/OneWire.h>

#include <configuration.h>
#include <tytherm.h>

Timer counterTimer;
void counter_loop();
unsigned long counter = 0;

void connectOk();
void connectFail();

void init()
{
	spiffs_mount(); // Mount file system, in order to work with files
	Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
	Serial.systemDebugOutput(false); // Debug output to serial
	Serial.commandProcessing(false);

	//SET higher CPU freq & disable wifi sleep
	system_update_cpu_freq(SYS_CPU_160MHZ);
	wifi_set_sleep_type(NONE_SLEEP_T);

	ActiveConfig = loadConfig();

	if (ActiveConfig.StaEnable)
	{
		WifiStation.waitConnection(StaConnectOk, StaConnectTimeout, StaConnectFail); // We recommend 20+ seconds for connection timeout at start
		WifiStation.enable(true);
		WifiStation.config(ActiveConfig.StaSSID, ActiveConfig.StaPassword);
	}
	else
	{
		WifiStation.enable(false);
	}

	WifiAccessPoint.config("TyTherm", "20040229", AUTH_WPA2_PSK);
	WifiAccessPoint.enable(true);
	startWebServer();

	counterTimer.initializeMs(1000, counter_loop).start();
}

void counter_loop()
{
	counter++;
}

void StaConnectOk()
{
	Serial.println("connected to AP");
	WifiAccessPoint.enable(false);
}

void StaConnectFail()
{
	Serial.println("connection FAILED");
	WifiStation.disconnect();
	WifiAccessPoint.config("TyTherm", "20040229", AUTH_WPA2_PSK);
	WifiAccessPoint.enable(true);
}
