'use strict';

//var appStatus
//var binStates;
//var tempsensors;
//var tempsensorsHome;

//import websocket from './websocket';
import AppStatusClass from 'appStatus';
//import BinStatesClass from 'binStates';
import { initWS, websocket, wsEnablers, wsBinProcessors } from 'websocket';
import TempsensorsClass from 'tempsensors';

//Here we put some initial code which starts after DOM loaded
function onDocumentRedy() {
//	tempsensors = new TempsensorsClass('/temperature.json', 0);
//	tempsensors.enable(true);
//	setInterval(function () { tempsensors.wsGetAllTemperatures(); }, 5000);
//	
//	tempsensorsHome = new TempsensorsClass('/temperatureHome.json', 1);
//	tempsensorsHome.enable(true);
//	setInterval(function () { tempsensorsHome.wsGetAllTemperatures(); }, 5000);

	var appStatus = new AppStatusClass();
//	var binStates = new BinStatesClass();
	
	wsEnablers.push(appStatus.enable.bind(appStatus));
//	wsEnablers.push(binStates.enableStates.bind(binStates));
//	wsEnablers.push(binStates.enableButtons.bind(binStates));
	
	wsBinProcessors[AppStatusClass.sysId] = appStatus.wsBinProcess.bind(appStatus);
//	wsBinProcessors[BinStatesClass.sysId] = binStates.wsBinProcess.bind(binStates);
	
	var tempsensors = new TempsensorsClass('/temperature.json', 0);
	tempsensors.enable(true);
	setInterval(function () { tempsensors.wsGetAllTemperatures(); }, 5000);
	
//	var tempsensorsHome = new TempsensorsClass('/temperatureHome.json', 1);
//	tempsensorsHome.enable(true);
//	setInterval(function () { tempsensorsHome.wsGetAllTemperatures(); }, 5000);
	
	initWS();
}

document.addEventListener('DOMContentLoaded', onDocumentRedy);