'use strict';

//wsBinProtocol constants
var wsBinConst = {
//Frame header offsets
	wsCmd			: 0, //Command type
	wsSysId			: 1, //target sysId
	wsSubCmd		: 2, //sub-command type
	wsPayLoadStart	: 3,
	//alternatively if we need argument to Get value
	wsPayLoadStartGetSetArg	: 4,
	wsGetSetArg		: 3,

	reservedCmd		: 0,
	getCmd			: 1,
	setCmd			: 2,
	getResponse		: 3,
	setAck			: 4,
	setNack			: 5,

// sub-command
	scAppSetTime	: 1,
	scAppGetStatus	: 2,
	scAppConfigGet	: 3,
	scAppConfigSet	: 4,
// sub-commands for BinStateHttpClass sysId=2 and BinStatesHttpClass sysId=3
	scBinStateGetName	: 1,
	scBinStateGetState	: 2,
	scBinStateSetState	: 3,
// sub-commands for BinStatesHttpClass sysId=3
	scBinStatesGetAll		: 10,
	scBinStatesGetAllStates	: 20,
	scBinStatesGetAllButtons	: 30,
// BinHttp State/Buttons base numbers
	uidHttpState		: 0,
	uidHttpButton		: 127
};


var wsBinCmd = {
	Get	: function (socket, sysId, subCmd) {
		var ab = new ArrayBuffer(3);
		var bin = new DataView(ab);
		
		bin.setUint8(wsBinConst.wsCmd, wsBinConst.getCmd);
		bin.setUint8(wsBinConst.wsSysId, sysId);
		bin.setUint8(wsBinConst.wsSubCmd, subCmd);
	
		socket.send(bin.buffer);
//		console.log.bind(console)(`wsGet sysId = ${sysId}, subCmd = ${subCmd}`);
	},
	SetArg: function (socket, sysId, subCmd, setArg, setValue) {
		var ab = new ArrayBuffer(5);
		var bin = new DataView(ab);
		
		bin.setUint8(wsBinConst.wsCmd, wsBinConst.setCmd);
		bin.setUint8(wsBinConst.wsSysId, sysId);
		bin.setUint8(wsBinConst.wsSubCmd, subCmd);
		bin.setUint8(wsBinConst.wsGetSetArg, setArg);
		bin.setUint8(wsBinConst.wsPayLoadStartGetSetArg, setValue);
	
		socket.send(bin.buffer);
//		console.log.bind(console)(`wsGet sysId = ${sysId}, subCmd = ${subCmd}`);
	}
}

//BinStateHttpClass

function BinStateClass (uid) {
	this.uid = uid;
	this._state = 0; //false
	this._name = "";
	this.render();
}

BinStateClass.prototype.wsGet = function (cmd) {
	var ab = new ArrayBuffer(3);
	var bin = new DataView(ab);
	
	bin.setUint8(wsBinConst.wsCmd, wsBinConst.getCmd);
	bin.setUint8(wsBinConst.wsSysId, 2); //BinStateHttpClass.sysId = 2
	bin.setUint8(wsBinConst.wsSubCmd, cmd);

	websocket.send(bin.buffer);
//	console.log.bind(console)(`wsGet cmd = ${cmd}`);
}

BinStateClass.prototype.wsGetName = function () {
	this.wsGet(1);
}

BinStateClass.prototype.wsGetState = function () {
	this.wsGet(2);
}

BinStateClass.prototype.wsSetState = function (state) {
	wsBinCmd.SetArg(websocket, 2, wsBinConst.scBinStateSetState, this.uid, state);
}

BinStateClass.prototype.wsGotName = function (bin) {
	var strBuffer = new Uint8Array(bin.byteLength -(wsBinConst.wsPayLoadStart + 1));
	console.log.bind(console)(`uid = ${this.uid}, bin.byteLength = ${bin.byteLength}`);

    for (var i = 0; i < strBuffer.length; i++) {
        strBuffer[i] = bin.getUint8(wsBinConst.wsPayLoadStart + 1 + i);
//        console.log.bind(console)(`uid = ${this.uid}, strBuffer[${i}] = ${bin.getUint8(wsBinConst.wsPayLoadStart + 1 + i)}`);
    }

    this._name = new TextDecoder().decode(strBuffer)
    this.renderName();
}

BinStateClass.prototype.wsGotState = function (bin) {
	this._state = bin.getUint8(wsBinConst.wsPayLoadStart + 1, true);
	this.renderState();
}

BinStateClass.prototype.render = function () {
	if ( this.isState() ) {
		var t = document.querySelector('#BinStateHttpClass');
		var clone = document.importNode(t.content, true);
//		clone.querySelector('#binState').textContent = this._name;
		clone.querySelector('#binStateDiv').id = `binStateDiv${this.uid}`;
		clone.querySelector('#binStatePanel').id = `binStatePanel${this.uid}`;
		clone.querySelector('#binState').id = `binState${this.uid}`
		var container = document.getElementById("Container-BinStateHttpClassStates");
	}
			
	if ( this.isButton() ) {
		var t = document.querySelector('#BinStateHttpClassButton');
		var clone = document.importNode(t.content, true);
//		clone.querySelector('#binStateButton').textContent = this._name;
		clone.querySelector('#binStateButtonDiv').id = `binStateButtonDiv${this.uid}`
		clone.querySelector('#binStateButton').addEventListener('mousedown', this);
		clone.querySelector('#binStateButton').addEventListener('mouseup', this);
		clone.querySelector('#binStateButton').id = `binStateButton${this.uid}`
		var container = document.getElementById("Container-BinStateHttpClassButtons");
	}
	
	container.appendChild(clone);	
}

BinStateClass.prototype.renderName = function () {
	if ( this.isState() ) {
		document.querySelector(`#binState${this.uid}`).textContent = this._name;
	}
	
	if ( this.isButton() ) {
		document.querySelector(`#binStateButton${this.uid}`).textContent = this._name;
	}
}

BinStateClass.prototype.renderState = function () {
	if ( this.isState()) {
	    	var panel = document.querySelector(`#binStatePanel${this.uid}`);
	    	
	    	if (this._state) {
	    		panel.classList.remove("panel-primary");
	    		panel.classList.add("panel-danger");	
	    	} else {
	    		panel.classList.remove("panel-danger");
	    		panel.classList.add("panel-primary");
	    	}
    	}
    	
    	if ( this.isButton()) {
    		var panel = document.querySelector(`#binStateButton${this.uid}`);
	    	
	    	if (this._state) {
	    		panel.classList.remove("btn-primary");
	    		panel.classList.add("btn-warning");	
	    	} else {
	    		panel.classList.remove("btn-warning");
	    		panel.classList.add("btn-primary");
	    	}
    	}	
}

BinStateClass.prototype.remove = function () {
		var selector = this.isState() ?  `#binStateDiv${this.uid}` : `#binStateButtonDiv${this.uid}`

		var removeElement = document.querySelector(selector);
		this.removeChilds(removeElement);
		removeElement.remove();
}

BinStateClass.prototype.removeChilds = function (node) {
    var last;
    while (last = node.lastChild) node.removeChild(last);
}

BinStateClass.prototype.wsBinProcess = function (bin) {
	var subCmd = bin.getUint8(wsBinConst.wsSubCmd);
	
	if ( subCmd == wsBinConst.scBinStateGetName) {
		this.wsGotName(bin);
	}
	
	if ( subCmd == wsBinConst.scBinStateGetState) {
		this.wsGotState(bin);
	}
	
}

BinStateClass.prototype.isButton = function () { return this.uid >= wsBinConst.uidHttpButton; }
BinStateClass.prototype.isState = function () { return this.uid < wsBinConst.uidHttpButton; }

BinStateClass.prototype.handleEvent = function(event) {
	switch(event.type) {
		case 'mousedown':
	        this.wsSetState(1);
	        break;
        case 'mouseup':
            this.wsSetState(0);
            break;
	}
}

//BinStatesHttpClass
// @param states - render/process states
// @param  buttons - render/process buttons

function BinStatesClass () {
	this._binStatesHttp = {};
	this._statesEnable = false;
	this._buttonsEnable = false;
}

BinStatesClass.prototype.enableStates = function( statesEnable ) {
	if ( statesEnable != this._statesEnable ) {
		this._statesEnable = statesEnable;
		if (! this._statesEnable) {
			var self = this
			Object.keys(this._binStatesHttp).forEach(function(uid) {
				if ( self.isState(uid) ) {
					self._binStatesHttp[uid].remove();
					delete self._binStatesHttp[uid];
				}
			});
		} else {
			this.wsGetAllStates();
		}	
	}
}

BinStatesClass.prototype.enableButtons = function( buttonsEnable ) {
	if ( buttonsEnable != this._buttonsEnable ) {
		this._buttonsEnable = buttonsEnable;
		if (! this._buttonsEnable) {
			var self = this
			Object.keys(this._binStatesHttp).forEach(function(uid) {
				if ( self.isButton(uid) ) {
					self._binStatesHttp[uid].remove();
					delete self._binStatesHttp[uid];
				}
			});
		} else {
			this.wsGetAllButtons();
		}	
	}
}

BinStatesClass.prototype.wsGetAll = function() {
	wsBinCmd.Get(websocket, 2, wsBinConst.scBinStatesGetAll);
}

BinStatesClass.prototype.wsGetAllStates = function() {
	wsBinCmd.Get(websocket, 2, wsBinConst.scBinStatesGetAllStates);
}

BinStatesClass.prototype.wsGetAllButtons = function() {
	wsBinCmd.Get(websocket, 2, wsBinConst.scBinStatesGetAllButtons);
}

BinStatesClass.prototype.wsBinProcess = function (bin) {
	var subCmd = bin.getUint8(wsBinConst.wsSubCmd);
	var uid = bin.getUint8(wsBinConst.wsPayLoadStart);
	
	if ( (this.isState(uid) && this._statesEnable) || (this.isButton(uid) && this._buttonsEnable ) ) {
		if (subCmd == wsBinConst.scBinStateGetName) {
			if ( !this._binStatesHttp.hasOwnProperty(uid) ) {
				this._binStatesHttp[uid] = new BinStateClass(uid);
			}
			this._binStatesHttp[uid].wsGotName(bin);
		}
		
		if (subCmd == wsBinConst.scBinStateGetState) {
			this._binStatesHttp[uid].wsGotState(bin);
		}
	}
	
	
}

BinStatesClass.prototype.isButton = function (uid) { return uid >= wsBinConst.uidHttpButton; }
BinStatesClass.prototype.isState = function (uid) { return uid < wsBinConst.uidHttpButton; }

function AppStatusClass() {
	this._counter = 0;
	this._timestamp = 0;
	this._dateStr = "";
	this._timer = 0;
	this._enable = false;
}

AppStatusClass.prototype.wsGetAppStatus = function() {
	wsBinCmd.Get(websocket, 1, wsBinConst.scAppGetStatus);
}

AppStatusClass.prototype.wsBinProcess = function (bin) {
	var subCmd = bin.getUint8(wsBinConst.wsSubCmd);
	if (subCmd == wsBinConst.scAppGetStatus) {
		this._counter = bin.getUint32(wsBinConst.wsPayLoadStart, true);
    	this._timestamp = bin.getUint32(wsBinConst.wsPayLoadStart + 4, true);
		var d = new Date();
		d.setTime(this._timestamp * 1000);
		this._dateStr = d.toLocaleString().replace(/,\ /,'<br>');
		this.renderStatus();
	}
}

AppStatusClass.prototype.render = function () {
	var t = document.querySelector('#AppStatusClass');
	var clone = document.importNode(t.content, true);
	var container = document.getElementById("Container-AppStatusClass");
	container.appendChild(clone);
}

AppStatusClass.prototype.renderStatus = function () {
	document.querySelector('#AppStatusClassCounter').textContent = this._counter;
	document.querySelector('#AppStatusClassDateTime').innerHTML = this._dateStr;
}

AppStatusClass.prototype.remove = function () {
		var removeElement = document.querySelector('#Container-AppStatusClass');
		this.removeChilds(removeElement);
}

AppStatusClass.prototype.removeChilds = function (node) {
    var last;
    while (last = node.lastChild) node.removeChild(last);
}

AppStatusClass.prototype.enable = function( enable ) {
	if ( enable != this._enable ) {
		this._enable = enable;
		if (! this._enable) {
			clearInterval(this._timer);
			this.remove();
		} else {
			this.render();
			this.wsGetAppStatus()
			this._timer = setInterval(this.wsGetAppStatus, 5000);
		}	
	}
}

//AppConfigClass
function AppConfigClass() {
	this._targetTemp = 0;
	this._targetTempDelta = 0;
	this._valveStepTime = 0;
	this._valveEdgeTime = 0;
	this._termostatUpdateTime = 0;
	this._enable = false;
}

AppConfigClass.prototype.wsSetAppConfig = function () {
		var ab = new ArrayBuffer(wsBinConst.wsPayLoadStart + 2 + 2 + 2 + 2 + 2);
		var bin = new DataView(ab);
		
		bin.setUint8(wsBinConst.wsCmd, wsBinConst.setCmd);
		bin.setUint8(wsBinConst.wsSysId, 1); //App sysid
		bin.setUint8(wsBinConst.wsSubCmd, wsBinConst.scAppConfigSet);
		bin.setUint16(wsBinConst.wsPayLoadStart, this._targetTemp * 100, true);
		bin.setUint16(wsBinConst.wsPayLoadStart + 2, this._targetTempDelta * 100, true);
		bin.setUint16(wsBinConst.wsPayLoadStart + 4, this._valveStepTime, true);
		bin.setUint16(wsBinConst.wsPayLoadStart + 6, this._valveEdgeTime, true);
		bin.setUint16(wsBinConst.wsPayLoadStart + 8, this._termostatUpdateTime, true);
	
		websocket.send(bin.buffer);
//		console.log.bind(console)(`wsSetAppConfig`);
	}

AppConfigClass.prototype.wsGetAppConfig = function() {
	wsBinCmd.Get(websocket, 1, wsBinConst.scAppConfigGet);
}

AppConfigClass.prototype.wsBinProcess = function (bin) {
	var subCmd = bin.getUint8(wsBinConst.wsSubCmd);
	if (subCmd == wsBinConst.scAppConfigGet) {
		this._targetTemp = bin.getUint16(wsBinConst.wsPayLoadStart, true) / 100.0;
    	this._targetTempDelta = bin.getUint16(wsBinConst.wsPayLoadStart + 2, true) / 100.0;
    	this._valveStepTime = bin.getUint16(wsBinConst.wsPayLoadStart + 4, true);
    	this._valveEdgeTime = bin.getUint16(wsBinConst.wsPayLoadStart + 6, true);
    	this._termostatUpdateTime = bin.getUint16(wsBinConst.wsPayLoadStart + 8, true);
		this.renderConfig();
	}
}

AppConfigClass.prototype.render = function () {
	var t = document.querySelector('#AppConfigClass');
	var clone = document.importNode(t.content, true);
	clone.querySelector('#AppConfigClassForm').addEventListener('submit', this);
	clone.querySelector('#AppConfigCancel').addEventListener('click', this);
	var container = document.getElementById("Container-AppConfigClass");
	container.appendChild(clone);
}

AppConfigClass.prototype.renderConfig = function () {
	document.querySelector('#AppConfigClass-_targetTemp').value = this._targetTemp;
	document.querySelector('#AppConfigClass-_targetTempDelta').value = this._targetTempDelta;
	document.querySelector('#AppConfigClass-_valveStepTime').value = this._valveStepTime;
	document.querySelector('#AppConfigClass-_valveEdgeTime').value = this._valveEdgeTime;
	document.querySelector('#AppConfigClass-_termostatUpdateTime').value = this._termostatUpdateTime;
}

AppConfigClass.prototype.remove = function () {
		var removeElement = document.querySelector('#Container-AppConfigClass');
		this.removeChilds(removeElement);
}

AppConfigClass.prototype.removeChilds = function (node) {
    var last;
    while (last = node.lastChild) node.removeChild(last);
}

AppConfigClass.prototype.enable = function( enable ) {
	if ( enable != this._enable ) {
		this._enable = enable;
		if (! this._enable) {
			this.remove();
		} else {
			this.render();
			this.wsGetAppConfig()
			this._timer = setInterval(this.wsGetAppStatus, 5000);
		}	
	}
}

AppConfigClass.prototype.getFormValues = function () {
	this._targetTemp = document.querySelector('#AppConfigClass-_targetTemp').value;
	this._targetTempDelta = document.querySelector('#AppConfigClass-_targetTempDelta').value;
	this._valveStepTime = document.querySelector('#AppConfigClass-_valveStepTime').value;
	this._valveEdgeTime = document.querySelector('#AppConfigClass-_valveEdgeTime').value;
	this._termostatUpdateTime = document.querySelector('#AppConfigClass-_termostatUpdateTime').value;
}

AppConfigClass.prototype.handleEvent = function(event) {
	switch(event.type) {
		case 'submit':
			event.preventDefault();
			this.getFormValues();
	        this.wsSetAppConfig();
	        break;
        case 'click':
            this.wsGetAppConfig();
            break;
	}
}

//TempsensorClass
function getJson(url) { return fetch(url).then(function(response){if (response.status >= 200 && response.status < 300) return response.json();})};

function TempsensorClass (uid, id) {
	this._id = id;
	this.uid = uid;
	this._temperature = 0;
	this._statusFlag = 0;
	this._name = uid;
	this.render();
}

TempsensorClass.statusFlags = { INVALID: 1, DISCONNECTED: 2};

TempsensorClass.prototype.wsGotName = function (uid) {
	this._name = uid;
    this.renderName();
}

TempsensorClass.prototype.wsGotTemperature = function (temperature, statusFlag) {
	this._temperature = temperature;
	this._statusFlag = statusFlag
	this.renderTemperature();
}

TempsensorClass.prototype.render = function () {
	var t = document.querySelector('#TempsensorClass');
	var clone = document.importNode(t.content, true);
	
	clone.querySelector('#TempsensorClassDiv').id = `TempsensorClassDiv${this._id + this.uid}`;
	clone.querySelector('#TempsensorClassPanel').id = `TempsensorClassPanel${this._id + this.uid}`;
	clone.querySelector('#TempsensorClassName').id = `TempsensorClassName${this._id + this.uid}`
	clone.querySelector('#TempsensorClassTemperature').id = `TempsensorClassTemperature${this._id + this.uid}`
		
	var container = document.getElementById("Container-TempsensorClassTemperatures");

	container.appendChild(clone);	
}

TempsensorClass.prototype.renderName = function () {
	document.querySelector(`#TempsensorClassName${this._id + this.uid}`).textContent = `Temperature #${this._name}`;
}

TempsensorClass.prototype.renderTemperature = function () {
	var panel = document.querySelector(`#TempsensorClassPanel${this._id + this.uid}`);
	
	if (this._statusFlag == 0 ) {
		panel.classList.remove("panel-danger");
		panel.classList.remove("panel-warning");
		panel.classList.add("panel-default");	
	} else if (this._statusFlag & TempsensorClass.statusFlags.DISCONNECTED ) {
		panel.classList.remove("panel-default");
		panel.classList.remove("panel-warning");
		panel.classList.add("panel-danger");
	} else if (this._statusFlag & TempsensorClass.statusFlags.INVALID ) {
		panel.classList.remove("panel-default");
		panel.classList.remove("panel-danger");
		panel.classList.add("panel-warning");
	}
	
	document.querySelector(`#TempsensorClassTemperature${this._id + this.uid}`).textContent = `${this._temperature} \xB0C`;
}

TempsensorClass.prototype.remove = function () {
	var removeElement = document.querySelector(`TempsensorClassDiv${this._id + this.uid}`);
	this.removeChilds(removeElement);
	removeElement.remove();
}

TempsensorClass.prototype.removeChilds = function (node) {
	var last;
	while (last = node.lastChild) node.removeChild(last);
}

// TempsensorsClass

function TempsensorsClass (url, id) {
	this._id = id;
	this._url = url;
	this._tempsensorsHttp = {};
	this._tempsensorsEnable = false;
}

TempsensorsClass.prototype.enable = function( tempsensorsEnable ) {
	if ( tempsensorsEnable != this._tempsensorsEnable ) {
		this._tempsensorsEnable = tempsensorsEnable;
		if (! this._tempsensorsEnable) {
			var self = this
			Object.keys(this._tempsensorsHttp).forEach(function(uid) {
				self._tempsensorsHttp[uid].remove();
				delete self._tempsensorsHttp[uid];
			});
		} else {
			this.wsGetAllTemperatures();
		}	
	}
}

TempsensorsClass.prototype.wsGetAllTemperatures = function () {
	var self = this;
	getJson(this._url)
	.then( function(temperatures) {
		Object.keys(temperatures).forEach(function(uid) {
			if ( !self._tempsensorsHttp.hasOwnProperty(uid) ) {
				self._tempsensorsHttp[uid] = new TempsensorClass(uid,self._id);
			}
			self._tempsensorsHttp[uid].wsGotName(uid);
			self._tempsensorsHttp[uid].wsGotTemperature(temperatures[uid].temperature, temperatures[uid].statusFlag);
		});
	});	
}