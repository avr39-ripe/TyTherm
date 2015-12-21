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
		cfg.StaSSID = String((const char*)network["StaSSID"]);
		cfg.StaPassword = String((const char*)network["StaPassword"]);
		cfg.StaEnable = network["StaEnable"];

		delete[] jsonString;
	}
	else
	{
		//Factory defaults if no config file present
		cfg.StaSSID = WIFI_SSID;
		cfg.StaPassword = WIFI_PWD;
	}
	return cfg;
}

void saveConfig(ThermConfig& cfg)
{
	DynamicJsonBuffer jsonBuffer;
	JsonObject& root = jsonBuffer.createObject();

	JsonObject& network = jsonBuffer.createObject();
	root["network"] = network;
	network["StaSSID"] = cfg.StaSSID.c_str();
	network["StaPassword"] = cfg.StaPassword.c_str();
	network["StaEnable"] = cfg.StaEnable;

	char buf[4048];
	root.prettyPrintTo(buf, sizeof(buf));
	fileSetContent(THERM_CONFIG_FILE, buf);
}


