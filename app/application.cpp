#include <user_config.h>
#include <tytherm.h>

Timer counterTimer;
void counter_loop();
unsigned long counter = 0;

OneWire ds(onewire_pin);
TempSensorOW tempSensor(ds, 4000);

void STADisconnect(String ssid, uint8_t ssid_len, uint8_t bssid[6], uint8_t reason);
void STAGotIP(IPAddress ip, IPAddress mask, IPAddress gateway);

void initialWifiConfig()
{
	struct softap_config apconfig;
	if(wifi_softap_get_config_default(&apconfig))
	{
		if (os_strncmp((const char *)apconfig.ssid, (const char *)"TyTherm", 32) != 0)
		{
			WifiAccessPoint.config("TyTherm", "20040229", AUTH_WPA2_PSK);

		}
		else
			Serial.printf("AccessPoint already configured.\n");
	}
	else
		Serial.println("AP NOT Started! - Get config failed!");

	if (WifiStation.getSSID().length() == 0)
	{
		WifiStation.config(WIFI_SSID, WIFI_PWD);
		WifiStation.enable(true, true);
		WifiAccessPoint.enable(false, true);
	}
	else
		Serial.printf("Station already configured.\n");
}

void init()
{
	spiffs_mount(); // Mount file system, in order to work with files
	Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
	Serial.systemDebugOutput(false);
	Serial.commandProcessing(false);

	//SET higher CPU freq & disable wifi sleep
	system_update_cpu_freq(SYS_CPU_160MHZ);
	wifi_set_sleep_type(NONE_SLEEP_T);

	initialWifiConfig(); //One-time WIFI setup

	ActiveConfig = loadConfig();

	WifiEvents.onStationDisconnect(STADisconnect);
	WifiEvents.onStationGotIP(STAGotIP);

	startWebServer();

	counterTimer.initializeMs(1000, counter_loop).start();

	ds.begin();
	tempSensor.start();

}

void counter_loop()
{
	counter++;
}

void STADisconnect(String ssid, uint8_t ssid_len, uint8_t bssid[6], uint8_t reason)
{
	Serial.printf("DELEGATE DISCONNECT - SSID: %s, REASON: %d\n", ssid.c_str(), reason);

	if (!WifiAccessPoint.isEnabled())
	{
		Serial.println("Starting OWN AP DELEGATE");
		WifiStation.disconnect();
		WifiAccessPoint.enable(true);
		WifiStation.connect();
	}
}

void STAGotIP(IPAddress ip, IPAddress mask, IPAddress gateway)
{
	Serial.printf("DELEGATE GOTIP - IP: %s, MASK: %s, GW: %s\n", ip.toString().c_str(),
																mask.toString().c_str(),
																gateway.toString().c_str());

	if (WifiAccessPoint.isEnabled())
	{
		WifiAccessPoint.enable(false);
	}

}
