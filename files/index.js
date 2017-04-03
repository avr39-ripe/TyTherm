'use strict';

var appStatus;
var binStates;
var tempsensors;
var tempsensorsHome;

//Websockets
var websocket;

function onOpen(evt) {
//	console.log.bind(console)("CONNECTED");
	
	appStatus = new AppStatusClass();
	appStatus.enable(true);
	
}

function onMessage(evt) {
//	console.log.bind(console)("Message recv: " + evt.data);
	if(evt.data instanceof ArrayBuffer) {
    	var bin = new DataView(evt.data);
    	
    	var cmd = bin.getUint8(wsBinConst.wsCmd);
    	var sysId = bin.getUint8(wsBinConst.wsSysId);
    	var subCmd = bin.getUint8(wsBinConst.wsSubCmd);
//    	console.log.bind(console)(`cmd = ${cmd}, sysId = ${sysId}, subCmd = ${subCmd}`);
    	
    	if ( cmd == wsBinConst.getResponse && sysId == 1 ) {
    		appStatus.wsBinProcess(bin);
    	}
    	
    	if ( cmd == wsBinConst.getResponse && ( sysId == 2 || sysId == 3) ) {
    		binStates.wsBinProcess(bin);
    	}
    		
  	} 
}

function onClose(evt) {
//	console.log.bind(console)("DISCONNECTED");
}

function onError(evt) {
//	console.log.bind(console)("ERROR: " + evt.data);
}

function initWS() {
	var wsUri = "ws://" + location.host + "/";
	websocket = new WebSocket(wsUri);
	websocket.onopen = function(evt) { onOpen(evt) };
	websocket.onclose = function(evt) { onClose(evt) };
	websocket.onmessage = function(evt) { onMessage(evt) };
	websocket.onerror = function(evt) { onError(evt) };
	websocket.binaryType = 'arraybuffer';
}


//Here we put some initial code which starts after DOM loaded
function onDocumentRedy() {
	tempsensors = new TempsensorsClass('/temperature.json', 0);
	tempsensors.enable(true);
	setInterval(function () { tempsensors.wsGetAllTemperatures(); }, 5000);

	initWS();
}

document.addEventListener('DOMContentLoaded', onDocumentRedy);
