/*
 * app.cpp
 *
 *  Created on: 30 марта 2016
 *      Author: shurik
 */
#include <app.h>
#include <tytherm.h>

//AppClass
void monitor(HttpRequest &request, HttpResponse &response); // Monitor via json some important params

void AppClass::init()
{
	tempSensor = new TempSensorsOW(ds, 4000);
	ds.begin();
	tempSensor->addSensor();

	ApplicationClass::init();

	webServer.addPath("/temperature.json",HttpPathDelegate(&TempSensors::onHttpGet,tempSensor));
}

void AppClass::start()
{
	ApplicationClass::start();
	tempSensor->start();
}

void AppClass::_loop()
{
	ApplicationClass::_loop();
}

