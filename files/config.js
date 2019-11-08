function get_config() {
    fetch('/config.json')
  	.then(function(response) {
      if (response.status >= 200 && response.status < 300) return response.json();
	})
	.then(function(configJson) {
		Object.keys(configJson).forEach(function(key) {
			document.getElementById(key).value = configJson[key];
		});
		if (configJson.StaEnable == 1) {
			document.getElementById('StaEnable').checked = true;
		} else {
			document.getElementById('StaEnable').checked = false;
		}
	});
}

function post_cfg(jsonData) {
	fetch('/config', {
		method: 'post',
		headers: {
			'Accept': 'application/json',
			'Content-Type': 'application/json; charset=utf-8'
			},
		body: JSON.stringify(jsonData)
	});
}

function post_netcfg(event) {
	event.preventDefault();
    var formData = {
            'StaSSID'                    :   document.getElementById('StaSSID').value,
            'StaPassword'                :   document.getElementById('StaPassword').value,
            'StaEnable'                  :   (document.getElementById('StaEnable').checked ? 1 : 0)
            };
    post_cfg(formData);
}

function post_config(event) {
	event.preventDefault();
	var formData = {
			'loopInterval'			:	document.getElementById('loopInterval').value,
			'updateURL'				:	document.getElementById('updateURL').value
			};
	post_cfg(formData);
}

function post_fw(action) {
// action should be either "update" or "switch"
	var json = {};
	json[action] = 1;
	
	fetch('/update', {
		method: 'post',
		headers: {
			'Accept': 'application/json',
			'Content-Type': 'application/json; charset=utf-8'
			},
		body: JSON.stringify(json)
	});
}

//Websockets
var websocket;
function onOpen(evt) {
	console.log.bind(console)("CONNECTED");
//	appConfig = new AppConfigClass();
//	appConfig.enable(true);;
//	websocket.send("Sming love WebSockets");
}

function onClose(evt) {
	console.log.bind(console)("DISCONNECTED");
}

function onMessage(evt) {
	console.log.bind(console)("Message recv: " + evt.data);
	if(evt.data instanceof ArrayBuffer) {
		var bin = new DataView(evt.data);
		
		var cmd = bin.getUint8(wsBinConst.wsCmd);
		var sysId = bin.getUint8(wsBinConst.wsSysId);
		var subCmd = bin.getUint8(wsBinConst.wsSubCmd);
		console.log.bind(console)(`cmd = ${cmd}, sysId = ${sysId}, subCmd = ${subCmd}`);
	}
}

function onError(evt) {
	console.log.bind(console)("ERROR: " + evt.data);
}

function initWS() {
	var wsUri = "ws://" + location.host + "/ws";
	websocket = new WebSocket(wsUri);
	websocket.onopen = function(evt) { onOpen(evt) };
	websocket.onclose = function(evt) { onClose(evt) };
	websocket.onmessage = function(evt) { onMessage (evt) };
	websocket.onerror = function(evt) { onError(evt) };
	websocket.binaryType = 'arraybuffer';
}

function closeWS() {
	websocket.close();
}

function sendTime(event) {
	event.preventDefault();
	var ab = new ArrayBuffer(8);
	var bin = new DataView(ab);
	var d = new Date();
	
	bin.setUint8(wsBinConst.wsCmd, wsBinConst.setCmd);
	bin.setUint8(wsBinConst.wsSysId, 1); //AppClass.sysId = 1
	bin.setUint8(wsBinConst.wsSubCmd, wsBinConst.scAppSetTime);
	
	bin.setUint32(wsBinConst.wsPayLoadStart,Math.round(d.getTime() / 1000),true);
	bin.setUint8(wsBinConst.wsPayLoadStart + 4, Math.abs(d.getTimezoneOffset()/60));	
	console.log.bind(console)(bin.getUint8(1),bin.getUint8(2),bin.getUint8(3),bin.getUint8(4));
	websocket.send(bin.buffer);
}

//Here we put some initial code which starts after DOM loaded
function onDocumentRedy() {
    //Init
	initWS();
	get_config();
    
    document.getElementById('form_netcfg').addEventListener('submit', post_netcfg);
    document.getElementById('netcfg_cancel').addEventListener('click', get_config);
    document.getElementById('form_settings').addEventListener('submit', post_config);
	document.getElementById('settings_cancel').addEventListener('click', get_config);
	document.getElementById('settings_update_fw').addEventListener('click', function() { post_fw("update"); });
	document.getElementById('settings_switch_fw').addEventListener('click', function() { post_fw("switch"); });
	document.getElementById('sync_datetime').addEventListener('click', sendTime);

}

document.addEventListener('DOMContentLoaded', onDocumentRedy);
