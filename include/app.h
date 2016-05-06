/*
 * app.h
 *
 *  Created on: 30 марта 2016
 *      Author: shurik
 */

#ifndef INCLUDE_APP_H_
#define INCLUDE_APP_H_
#include <application.h>

class AppClass : public ApplicationClass
{
public:
	virtual void init(); // Application initialization
	virtual void start(); // Application main-loop start/restart
protected:
	virtual void _loop(); // Application main loop function goes here
};



#endif /* INCLUDE_APP_H_ */
