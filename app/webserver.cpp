#include <user_config.h>
#include <SmingCore/SmingCore.h>

#include <configuration.h>
#include <tytherm.h>


bool serverStarted = false;
HttpServer server;

void onIndex(HttpRequest &request, HttpResponse &response)
{
	response.setCache(86400, true); // It's important to use cache for better performance.
	response.sendFile("index.html");
}

void onConfiguration(HttpRequest &request, HttpResponse &response)
{

	if (request.getRequestMethod() == RequestMethod::POST)
	{
		debugf("Update config");
		// Update config
		if (request.getPostParameter("SSID").length() > 0) // Network
		{
			ActiveConfig.NetworkSSID = request.getPostParameter("SSID");
			ActiveConfig.NetworkPassword = request.getPostParameter("Password");
			ActiveConfig.sta_enable = request.getPostParameter("sta_enable").toInt();

			if (ActiveConfig.sta_enable == 1)
			{
				WifiStation.config(ActiveConfig.NetworkSSID, ActiveConfig.NetworkPassword);
				WifiStation.enable(true);
			}
			else
			{
				WifiStation.enable(false);
			}

		}

		if (request.getBody() == NULL)
			Serial.println("NULL bodyBuf");
		else
		{
			Serial.print("HERE IS bodyBuf ! ");
			Serial.println(request.getBody());
			StaticJsonBuffer<200> jsonBuffer;
			JsonObject& root = jsonBuffer.parseObject(request.getBody());
			root.prettyPrintTo(Serial);

//			if (root["start_minutes"].success()) // Settings
//			{
//				ActiveConfig.start_minutes = root["start_minutes"];
//				ActiveConfig.stop_minutes = root["stop_minutes"];
//				ActiveConfig.cycle_duration = root["cycle_duration"];
//				ActiveConfig.cycle_interval = root["cycle_interval"];
//
//			}
		}
		saveConfig(ActiveConfig);
	//	response.redirect();
	}
	else
	{
		response.setCache(86400, true); // It's important to use cache for better performance.
		response.sendFile("config.html");
	}
}

void onConfiguration_json(HttpRequest &request, HttpResponse &response)
{
	JsonObjectStream* stream = new JsonObjectStream();
	JsonObject& json = stream->getRoot();

	json["SSID"] = ActiveConfig.NetworkSSID;
	json["Password"] = ActiveConfig.NetworkPassword;
	json["sta_enable"] = ActiveConfig.sta_enable;

	response.sendJsonObject(stream);
}
void onFile(HttpRequest &request, HttpResponse &response)
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

void onAJAXGetState(HttpRequest &request, HttpResponse &response)
{
	JsonObjectStream* stream = new JsonObjectStream();
	JsonObject& json = stream->getRoot();

	json["counter"] = counter;

	response.sendJsonObject(stream);
}


void startWebServer()
{
	if (serverStarted) return;

	server.listen(80);
	server.addPath("/", onIndex);
	server.addPath("/config", onConfiguration);
	server.addPath("/config.json", onConfiguration_json);
	server.addPath("/state", onAJAXGetState);
	server.setDefaultHandler(onFile);
	serverStarted = true;

	if (WifiStation.isEnabled())
		debugf("STA: %s", WifiStation.getIP().toString().c_str());
	if (WifiAccessPoint.isEnabled())
		debugf("AP: %s", WifiAccessPoint.getIP().toString().c_str());
}
