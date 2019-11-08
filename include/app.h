/*
 * app.h
 *
 *  Created on: 30 марта 2016
 *      Author: shurik
 */

#pragma once
#include <application.h>

class AppClass : public ApplicationClass
{
public:
	virtual void init(); // Application initialization
	virtual void start(); // Application main-loop start/restart
protected:
	virtual void _loop(); // Application main loop function goes here
};
