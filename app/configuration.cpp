#include "../include/configuration.h"

#include <SmingCore/SmingCore.h>

ThermConfig ActiveConfig;

ThermConfig loadConfig()
{
	DynamicJsonBuffer jsonBuffer;
	ThermConfig cfg;
	if (fileExist(THERM_CONFIG_FILE))
	{
		int size = fileGetSize(THERM_CONFIG_FILE);
		char* jsonString = new char[size + 1];
		fileGetContent(THERM_CONFIG_FILE, jsonString, size + 1);
		JsonObject& root = jsonBuffer.parseObject(jsonString);

		JsonObject& network = root["network"];
		cfg.NetworkSSID = String((const char*)network["ssid"]);
		cfg.NetworkPassword = String((const char*)network["password"]);
		cfg.sta_enable = network["sta_enable"];

		delete[] jsonString;
	}
	else
	{
		//Factory defaults if no config file present
		cfg.NetworkSSID = WIFI_SSID;
		cfg.NetworkPassword = WIFI_PWD;
	}
	return cfg;
}

void saveConfig(ThermConfig& cfg)
{
	DynamicJsonBuffer jsonBuffer;
	JsonObject& root = jsonBuffer.createObject();

	JsonObject& network = jsonBuffer.createObject();
	root["network"] = network;
	network["ssid"] = cfg.NetworkSSID.c_str();
	network["password"] = cfg.NetworkPassword.c_str();
	network["sta_enable"] = cfg.sta_enable;

	char buf[4048];
	root.prettyPrintTo(buf, sizeof(buf));
	fileSetContent(THERM_CONFIG_FILE, buf);
}


