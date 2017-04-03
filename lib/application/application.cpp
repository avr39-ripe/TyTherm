#include <application.h>

//ApplicationClass App;

void ApplicationClass::init()
{
	Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
	Serial.systemDebugOutput(true);
	Serial.commandProcessing(true);

	int slot = rboot_get_current_rom();
#ifndef DISABLE_SPIFFS
	if (slot == 0) {
#ifdef RBOOT_SPIFFS_0
//		debugf("trying to mount spiffs at %x, length %d", RBOOT_SPIFFS_0, SPIFF_SIZE);
		spiffs_mount_manual(RBOOT_SPIFFS_0, SPIFF_SIZE);
#else
//		debugf("trying to mount spiffs at %x, length %d", 0x100000, SPIFF_SIZE);
		spiffs_mount_manual(0x100000, SPIFF_SIZE);
#endif
	} else {
#ifdef RBOOT_SPIFFS_1
//		debugf("trying to mount spiffs at %x, length %d", RBOOT_SPIFFS_1, SPIFF_SIZE);
		spiffs_mount_manual(RBOOT_SPIFFS_1, SPIFF_SIZE);
#else
//		debugf("trying to mount spiffs at %x, length %d", 0x300000, SPIFF_SIZE);
		spiffs_mount_manual(0x300000, SPIFF_SIZE);
#endif
	}
#else
//	debugf("spiffs disabled");
#endif
//	spiffs_mount(); // Mount file system, in order to work with files

	_initialWifiConfig();

	loadConfig();

//	ntpClient = new NtpClient("pool.ntp.org", 300); //uncomment to enablentp update of system time
	SystemClock.setTimeZone(timeZone); //set time zone from config
	Serial.printf("Time zone: %d\n", timeZone);

	// Attach Wifi events handlers
	WifiEvents.onStationDisconnect(onStationDisconnectDelegate(&ApplicationClass::_STADisconnect, this));
	WifiEvents.onStationConnect(onStationConnectDelegate(&ApplicationClass::_STAConnect, this));
	WifiEvents.onStationAuthModeChange(onStationAuthModeChangeDelegate(&ApplicationClass::_STAAuthModeChange, this));
	WifiEvents.onStationGotIP(onStationGotIPDelegate(&ApplicationClass::_STAGotIP, this));

	// Web Sockets configuration
	webServer.enableWebSockets(true);
	webServer.setWebSocketConnectionHandler(WebSocketDelegate(&ApplicationClass::wsConnected,this));
	webServer.setWebSocketMessageHandler(WebSocketMessageDelegate(&ApplicationClass::wsMessageReceived,this));
	webServer.setWebSocketBinaryHandler(WebSocketBinaryDelegate(&ApplicationClass::wsBinaryReceived,this));
	webServer.setWebSocketDisconnectionHandler(WebSocketDelegate(&ApplicationClass::wsDisconnected,this));

	wsAddBinSetter(sysId, WebSocketBinaryDelegate(&ApplicationClass::wsBinSetter,this));
	wsAddBinGetter(sysId, WebSocketBinaryDelegate(&ApplicationClass::wsBinGetter,this));

	startWebServer();
}

void ApplicationClass::start()
{
	_loopTimer.initializeMs(loopInterval, TimerDelegate(&ApplicationClass::_loop, this)).start(true);
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
	userSTAGotIP(ip, mask, gateway);
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
//	webServer.addPath("/state.json",HttpPathDelegate(&ApplicationClass::_httpOnStateJson,this));
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
				loopInterval = root["loopInterval"];
				start(); // restart main application loop with new loopInterval setting
				needSave = true;
			}


			if (root["updateURL"].success()) // There is loopInterval parameter in json
			{
				updateURL = String((const char *)root["updateURL"]);
				needSave = true;
			}

			if (needSave)
			{
				saveConfig();
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
	json["loopInterval"] = loopInterval;
	json["updateURL"] = updateURL;

	response.sendJsonObject(stream);
}

void ApplicationClass::loadConfig()
{
	uint16_t strSize;

	Serial.printf("Try to load ApplicationClass bin cfg..\n");
	if (fileExist(_fileName))
	{
		Serial.printf("Will load ApplicationClass bin cfg..\n");
		file_t file = fileOpen(_fileName, eFO_ReadOnly);
		fileSeek(file, 0, eSO_FileStart);
		fileRead(file, &strSize, sizeof(strSize));
		uint8_t* updateURLBuffer = new uint8_t[strSize+1];
		fileRead(file, updateURLBuffer, strSize);
		updateURLBuffer[strSize] = 0;
		updateURL = (const char *)updateURLBuffer;
		fileRead(file, &loopInterval, sizeof(loopInterval));
		fileRead(file, &timeZone, sizeof(timeZone));

		_loadAppConfig(file); //load additional, child class config here

		fileClose(file);

		delete [] updateURLBuffer;
	}
}

void ApplicationClass::saveConfig()
{
	uint16_t strSize = updateURL.length();

	Serial.printf("Try to save ApplicationClass bin cfg..\n");
	file_t file = fileOpen(_fileName, eFO_CreateNewAlways | eFO_WriteOnly);
	fileWrite(file, &strSize, sizeof(strSize));
	fileWrite(file, updateURL.c_str(), strSize);
	fileWrite(file, &loopInterval, sizeof(loopInterval));
	fileWrite(file, &timeZone, sizeof(timeZone));

	_saveAppConfig(file); //save additional, child class config here

	fileClose(file);
}

void ApplicationClass::OtaUpdate_CallBack(rBootHttpUpdate& client, bool result) {

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
	otaUpdater->addItem(bootconf.roms[slot], updateURL + "rom0.bin");
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
		otaUpdater->addItem(RBOOT_SPIFFS_0, updateURL + "spiff_rom.bin");
	} else {
		otaUpdater->addItem(RBOOT_SPIFFS_1, updateURL + "spiff_rom.bin");
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
//WebSocket handling
void ApplicationClass::wsConnected(WebSocket& socket)
{
	Serial.printf("WS CONN!\n");
}

void ApplicationClass::wsDisconnected(WebSocket& socket)
{
	Serial.printf("WS DISCONN!\n");
}

void ApplicationClass::wsMessageReceived(WebSocket& socket, const String& message)
{
	Serial.printf("WS msg recv: %s\n", message.c_str());
}

void ApplicationClass::wsBinaryReceived(WebSocket& socket, uint8_t* data, size_t size)
{
	Serial.printf("WS bin data recv, size: %d\r\n", size);
	Serial.printf("Opcode: %d\n", data[0]);

	if ( data[wsBinConst::wsCmd] == wsBinConst::setCmd)
	{
		Serial.printf("wsSetCmd\n");
		if (_wsBinSetters.contains(data[wsBinConst::wsSysId]))
		{
			Serial.printf("wsSysId = %d setter called!\n", data[wsBinConst::wsSysId]);
			_wsBinSetters[data[wsBinConst::wsSysId]](socket, data, size);
		}
	}

	if ( data[wsBinConst::wsCmd] == wsBinConst::getCmd)
	{
		Serial.printf("wsGetCmd\n");
		if (_wsBinGetters.contains(data[wsBinConst::wsSysId]))
		{
			Serial.printf("wsSysId = %d getter called!\n", data[wsBinConst::wsSysId]);
			_wsBinGetters[data[wsBinConst::wsSysId]](socket, data, size);
		}
	}
}

void ApplicationClass::wsBinSetter(WebSocket& socket, uint8_t* data, size_t size)
{
	switch (data[wsBinConst::wsSubCmd])
	{
	case wsBinConst::scAppSetTime:
	{
		uint32_t timestamp = 0;
		os_memcpy(&timestamp, (&data[wsBinConst::wsPayLoadStart]), 4);
		if (timeZone != data[wsBinConst::wsPayLoadStart + 4])
		{
			timeZone = data[wsBinConst::wsPayLoadStart + 4];
			saveConfig();
			SystemClock.setTimeZone(timeZone);
		}
		SystemClock.setTime(timestamp, eTZ_UTC);
		break;
	}
	}
}

void ApplicationClass::wsBinGetter(WebSocket& socket, uint8_t* data, size_t size)
{
	uint8_t* buffer;
	switch (data[wsBinConst::wsSubCmd])
	{
	case wsBinConst::scAppGetStatus:
	{
		buffer = new uint8_t[wsBinConst::wsPayLoadStart + 4 + 4];
		buffer[wsBinConst::wsCmd] = wsBinConst::getResponse;
		buffer[wsBinConst::wsSysId] = sysId;
		buffer[wsBinConst::wsSubCmd] = wsBinConst::scAppGetStatus;

		DateTime now = SystemClock.now(eTZ_UTC);
		uint32_t timestamp = now.toUnixTime();
		os_memcpy((&buffer[wsBinConst::wsPayLoadStart]), &_counter, sizeof(_counter));
		os_memcpy((&buffer[wsBinConst::wsPayLoadStart + 4]), &timestamp, sizeof(timestamp));
		socket.sendBinary(buffer, wsBinConst::wsPayLoadStart + 4 + 4);
		delete buffer;
		break;
	}
	}
}

void ApplicationClass::wsAddBinSetter(uint8_t sysId, WebSocketBinaryDelegate wsBinSetterDelegate)
{
	_wsBinSetters[sysId] = wsBinSetterDelegate;
}

void ApplicationClass::wsAddBinGetter(uint8_t sysId, WebSocketBinaryDelegate wsBinGetterDelegate)
{
	_wsBinGetters[sysId] = wsBinGetterDelegate;
}
