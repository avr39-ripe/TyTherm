#include <application.h>
//#include <user_config.h>

//ApplicationClass App;

//void init()
//{
//	App.init();
//	App.start();
//}

void ApplicationClass::init()
{
	Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
	Serial.systemDebugOutput(true);
	Serial.commandProcessing(true);

	int slot = rboot_get_current_rom();
#ifndef DISABLE_SPIFFS
	if (slot == 0) {
#ifdef RBOOT_SPIFFS_0
		debugf("trying to mount spiffs at %x, length %d", RBOOT_SPIFFS_0 + 0x40200000, SPIFF_SIZE);
		spiffs_mount_manual(RBOOT_SPIFFS_0 + 0x40200000, SPIFF_SIZE);
#else
		debugf("trying to mount spiffs at %x, length %d", 0x40300000, SPIFF_SIZE);
		spiffs_mount_manual(0x40300000, SPIFF_SIZE);
#endif
	} else {
#ifdef RBOOT_SPIFFS_1
		debugf("trying to mount spiffs at %x, length %d", RBOOT_SPIFFS_1 + 0x40200000, SPIFF_SIZE);
		spiffs_mount_manual(RBOOT_SPIFFS_1 + 0x40200000, SPIFF_SIZE);
#else
		debugf("trying to mount spiffs at %x, length %d", 0x40500000, SPIFF_SIZE);
		spiffs_mount_manual(0x40500000, SPIFF_SIZE);
#endif
	}
#else
	debugf("spiffs disabled");
#endif
//	spiffs_mount(); // Mount file system, in order to work with files

	_initialWifiConfig();

	Config.load();

	// Attach Wifi events handlers
	WifiEvents.onStationDisconnect(onStationDisconnectDelegate(&ApplicationClass::_STADisconnect, this));
	WifiEvents.onStationConnect(onStationConnectDelegate(&ApplicationClass::_STAConnect, this));
	WifiEvents.onStationAuthModeChange(onStationAuthModeChangeDelegate(&ApplicationClass::_STAAuthModeChange, this));
	WifiEvents.onStationGotIP(onStationGotIPDelegate(&ApplicationClass::_STAGotIP, this));

	startWebServer();
}

void ApplicationClass::start()
{
	_loopTimer.initializeMs(Config.loopInterval, TimerDelegate(&ApplicationClass::_loop, this)).start(true);
	_loop();
}

void ApplicationClass::stop()
{
	_loopTimer.stop();
}

void ApplicationClass::_loop()
{
	_counter++;
}

void ApplicationClass::_initialWifiConfig()
{
// Set DHCP hostname to WebAppXXXX where XXXX is last 4 digits of MAC address
	String macDigits =  WifiStation.getMAC().substring(8,12);
	macDigits.toUpperCase();
	WifiStation.setHostname("WebApp" + macDigits);

// One-time set own soft Access Point SSID and PASSWORD and save it into configuration area
// This part of code will run ONCE after application flash into the ESP
	if(WifiAccessPoint.getSSID() != WIFIAP_SSID)
	{
		WifiAccessPoint.config(WIFIAP_SSID, WIFIAP_PWD, AUTH_WPA2_PSK);
		WifiAccessPoint.enable(true, true);
	}
	else
		Serial.printf("AccessPoint already configured.\n");

// One-time set initial SSID and PASSWORD for Station mode and save it into configuration area
// This part of code will run ONCE after application flash into the ESP if there is no
// pre-configured SSID and PASSWORD found in configuration area. Later you can change
// Station SSID and PASSWORD from Web UI and they will NOT overwrite by this part of code

	if (WifiStation.getSSID().length() == 0)
	{
		WifiStation.config(WIFI_SSID, WIFI_PWD);
		WifiStation.enable(true, true);
		WifiAccessPoint.enable(false, true);
	}
	else
		Serial.printf("Station already configured.\n");
}

void ApplicationClass::_STADisconnect(String ssid, uint8_t ssid_len, uint8_t bssid[6], uint8_t reason)
{
	debugf("DISCONNECT - SSID: %s, REASON: %d\n", ssid.c_str(), reason);

	_reconnectTimer.stop();
	if (!WifiAccessPoint.isEnabled())
	{
		debugf("Starting OWN AP");
		WifiStation.disconnect();
		WifiAccessPoint.enable(true);
		WifiStation.connect();
	}
}

void ApplicationClass::_STAAuthModeChange(uint8_t oldMode, uint8_t newMode)
{
	debugf("AUTH MODE CHANGE - OLD MODE: %d, NEW MODE: %d\n", oldMode, newMode);

	if (!WifiAccessPoint.isEnabled())
	{
		debugf("Starting OWN AP");
		WifiStation.disconnect();
		WifiAccessPoint.enable(true);
		WifiStation.connect();
	}
}

void ApplicationClass::_STAGotIP(IPAddress ip, IPAddress mask, IPAddress gateway)
{
	debugf("GOTIP - IP: %s, MASK: %s, GW: %s\n", ip.toString().c_str(),
																mask.toString().c_str(),
																gateway.toString().c_str());
	_reconnectTimer.stop();
	if (WifiAccessPoint.isEnabled())
	{
		debugf("Shutdown OWN AP");
		WifiAccessPoint.enable(false);
	}
	// Add commands to be executed after successfully connecting to AP and got IP from it
}

void ApplicationClass::_STAConnect(String ssid, uint8_t ssid_len, uint8_t bssid[6], uint8_t channel)
{
	debugf("DELEGATE CONNECT - SSID: %s, CHANNEL: %d\n", ssid.c_str(), channel);

	wifi_station_dhcpc_set_maxtry(128);
	_reconnectTimer.initializeMs(35000, TimerDelegate(&ApplicationClass::_STAReconnect,this)).start();
	// Add commands to be executed after successfully connecting to AP
}

void ApplicationClass::_STAReconnect()
{
	WifiStation.disconnect();
	WifiStation.connect();
}

void ApplicationClass::startWebServer()
{
	if (_webServerStarted) return;

	webServer.listen(80);
	webServer.addPath("/",HttpPathDelegate(&ApplicationClass::_httpOnIndex,this));
	webServer.addPath("/config",HttpPathDelegate(&ApplicationClass::_httpOnConfiguration,this));
	webServer.addPath("/config.json",HttpPathDelegate(&ApplicationClass::_httpOnConfigurationJson,this));
	webServer.addPath("/state.json",HttpPathDelegate(&ApplicationClass::_httpOnStateJson,this));
	webServer.addPath("/update",HttpPathDelegate(&ApplicationClass::_httpOnUpdate,this));
	webServer.setDefaultHandler(HttpPathDelegate(&ApplicationClass::_httpOnFile,this));
	_webServerStarted = true;

	if (WifiStation.isEnabled())
		debugf("STA: %s", WifiStation.getIP().toString().c_str());
	if (WifiAccessPoint.isEnabled())
		debugf("AP: %s", WifiAccessPoint.getIP().toString().c_str());
}

void ApplicationClass::_httpOnFile(HttpRequest &request, HttpResponse &response)
{
	String file = request.getPath();
	if (file[0] == '/')
		file = file.substring(1);

	if (file[0] == '.')
		response.forbidden();
	else
	{
		response.setCache(86400, true); // It's important to use cache for better performance.
		response.sendFile(file);
	}
}

void ApplicationClass::_httpOnIndex(HttpRequest &request, HttpResponse &response)
{
	response.setCache(86400, true); // It's important to use cache for better performance.
	response.sendFile("index.html");
}

void ApplicationClass::_httpOnStateJson(HttpRequest &request, HttpResponse &response)
{
	JsonObjectStream* stream = new JsonObjectStream();
	JsonObject& json = stream->getRoot();

	json["counter"] = _counter;

	response.sendJsonObject(stream);
}

void ApplicationClass::_httpOnConfiguration(HttpRequest &request, HttpResponse &response)
{

	if (request.getRequestMethod() == RequestMethod::POST)
	{
		debugf("Update configuration\n");

		if (request.getBody() == NULL)
		{
			debugf("Empty Request Body!\n");
			return;
		}
		else // Request Body Not Empty
		{
//Uncomment next line for extra debuginfo
//			Serial.printf(request.getBody());
			uint8_t needSave = false;
			DynamicJsonBuffer jsonBuffer;
			JsonObject& root = jsonBuffer.parseObject(request.getBody());
//Uncomment next line for extra debuginfo
//			root.prettyPrintTo(Serial);

			//Mandatory part to setup WIFI
			_handleWifiConfig(root);

			//Application config processing

			if (root["loopInterval"].success()) // There is loopInterval parameter in json
			{
				Config.loopInterval = root["loopInterval"];
				start(); // restart main application loop with new loopInterval setting
				needSave = true;
			}


			if (root["updateURL"].success()) // There is loopInterval parameter in json
			{
				Config.updateURL = String((const char *)root["updateURL"]);
				needSave = true;
			}

			if (needSave)
			{
				Config.save();
			}
		} // Request Body Not Empty
	} // Request method is POST
	else
	{
		response.setCache(86400, true); // It's important to use cache for better performance.
		response.sendFile("config.html");
	}
}

void ApplicationClass::_handleWifiConfig(JsonObject& root)
{
	String StaSSID = root["StaSSID"].success() ? String((const char *)root["StaSSID"]) : "";
	String StaPassword = root["StaPassword"].success() ? String((const char *)root["StaPassword"]) : "";
	uint8_t StaEnable = root["StaEnable"].success() ? root["StaEnable"] : 255;

	if (StaEnable != 255) // WiFi Settings
	{
		if (StaEnable)
		{
			if (WifiStation.isEnabled())
			{
				WifiAccessPoint.enable(false);
			}
			else
			{
				WifiStation.enable(true, true);
				WifiAccessPoint.enable(false, true);
			}
			if (WifiStation.getSSID() != StaSSID || (WifiStation.getPassword() != StaPassword && StaPassword.length() >= 8))
			{
				WifiStation.config(StaSSID, StaPassword);
				WifiStation.connect();
			}
		}
		else
		{
				WifiStation.enable(false, true);
				WifiAccessPoint.enable(true, true);
				WifiAccessPoint.config(WIFIAP_SSID, WIFIAP_PWD, AUTH_WPA2_PSK);
		}
	} //Wifi settings
}

void ApplicationClass::_httpOnConfigurationJson(HttpRequest &request, HttpResponse &response)
{
	JsonObjectStream* stream = new JsonObjectStream();
	JsonObject& json = stream->getRoot();

	//Mandatory part of WIFI Station SSID & Station mode enable config
	json["StaSSID"] = WifiStation.getSSID();
	json["StaEnable"] = WifiStation.isEnabled() ? 1 : 0;

	//Application configuration parameters
	json["loopInterval"] = Config.loopInterval;
	json["updateURL"] = Config.updateURL;

	response.sendJsonObject(stream);
}

void ApplicationConfig::load()
{
	DynamicJsonBuffer jsonBuffer;

	if (fileExist(_fileName))
	{
		int size = fileGetSize(_fileName);
		char* jsonString = new char[size + 1];
		fileGetContent(_fileName, jsonString, size + 1);

		JsonObject& root = jsonBuffer.parseObject(jsonString);

		loopInterval = root["loopInterval"];
		updateURL = String((const char *)root["updateURL"]);

		delete[] jsonString;
	}
	else
	{
		//Factory defaults if no config file present
		loopInterval = 1000; // 1 second
		updateURL = "http://192.168.31.181/";
	}
}

void ApplicationConfig::save()
{
	DynamicJsonBuffer jsonBuffer;
	JsonObject& root = jsonBuffer.createObject();

	root["loopInterval"] = loopInterval;
	root["updateURL"] = updateURL;

	String buf;
	root.printTo(buf);
	fileSetContent(_fileName, buf);
}

void ApplicationClass::OtaUpdate_CallBack(bool result) {

	Serial.println("In callback...");
	if(result == true) {
		// success
		uint8 slot;
		slot = rboot_get_current_rom();
		if (slot == 0) slot = 1; else slot = 0;
		// set to boot new rom and then reboot
		Serial.printf("Firmware updated, rebooting to rom %d...\r\n", slot);
		rboot_set_current_rom(slot);
		System.restart();
	} else {
		// fail
		Serial.println("Firmware update failed!");
	}
}

void ApplicationClass::OtaUpdate() {

	uint8 slot;
	rboot_config bootconf;

	Serial.println("Updating...");

	// need a clean object, otherwise if run before and failed will not run again
	if (otaUpdater) delete otaUpdater;
	otaUpdater = new rBootHttpUpdate();

	// select rom slot to flash
	bootconf = rboot_get_config();
	slot = bootconf.current_rom;
	if (slot == 0) slot = 1; else slot = 0;

#ifndef RBOOT_TWO_ROMS
	// flash rom to position indicated in the rBoot config rom table
	otaUpdater->addItem(bootconf.roms[slot], Config.updateURL + "rom0.bin");
#else
	// flash appropriate rom
	if (slot == 0) {
		otaUpdater->addItem(bootconf.roms[slot], ROM_0_URL);
	} else {
		otaUpdater->addItem(bootconf.roms[slot], ROM_1_URL);
	}
#endif

#ifndef DISABLE_SPIFFS
	// use user supplied values (defaults for 4mb flash in makefile)
	if (slot == 0) {
		otaUpdater->addItem(RBOOT_SPIFFS_0, Config.updateURL + "spiff_rom.bin");
	} else {
		otaUpdater->addItem(RBOOT_SPIFFS_1, Config.updateURL + "spiff_rom.bin");
	}
#endif

	// request switch and reboot on success
	//otaUpdater->switchToRom(slot);
	// and/or set a callback (called on failure or success without switching requested)
	otaUpdater->setCallback(otaUpdateDelegate(&ApplicationClass::OtaUpdate_CallBack,this));

	// start update
	otaUpdater->start();
}

void ApplicationClass::Switch() {
	uint8 before, after;
	before = rboot_get_current_rom();
	if (before == 0) after = 1; else after = 0;
	Serial.printf("Swapping from rom %d to rom %d.\r\n", before, after);
	rboot_set_current_rom(after);
	Serial.println("Restarting...\r\n");
	System.restart();
}

void ApplicationClass::_httpOnUpdate(HttpRequest &request, HttpResponse &response)
{
	if (request.getRequestMethod() == RequestMethod::POST)
		{
			debugf("Update POST request\n");

			if (request.getBody() == NULL)
			{
				debugf("Empty Request Body!\n");
				return;
			}
			else // Request Body Not Empty
			{
	//Uncomment next line for extra debuginfo
	//			Serial.printf(request.getBody());
				DynamicJsonBuffer jsonBuffer;
				JsonObject& root = jsonBuffer.parseObject(request.getBody());
	//Uncomment next line for extra debuginfo
				root.prettyPrintTo(Serial);


				//Application config processing

				if (root["update"].success()) // There is loopInterval parameter in json
				{
					OtaUpdate();
				}
				if (root["switch"].success()) // There is loopInterval parameter in json
				{
					Switch();
				}
			} // Request Body Not Empty
		} // Request method is POST
}
