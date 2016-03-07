// function get_config() {
	// $.getJSON('/config.json',
			// function(data) {
				// $.each(data, function(key, value){
            		// document.getElementById(key).value = value;
            	// if (data.StaEnable == 1) {
            		// document.getElementById('StaEnable').checked = true;
            	// }
            	// else
            		// document.getElementById('StaEnable').checked = false;
        		// });
            // });
// }
function get_config() {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', '/config.json', true);

    xhr.send();

    xhr.onreadystatechange = function() {
        
        if (this.readyState != 4) return;
        if (this.status == 200) {
            if (this.responseText.length > 0) {
                var configJson = JSON.parse(this.responseText);
                Object.keys(configJson).forEach(function(key) {
                    document.getElementById(key).value = configJson[key];
                });
                if (configJson.StaEnable == 1) {
                    document.getElementById('StaEnable').checked = true;
                }
                else
                    document.getElementById('StaEnable').checked = false;
            }
        }
   };
}
    

// function post_netcfg(event) {
	// event.preventDefault();
	// var formData = {
			// 'StaSSID'					:	document.getElementById('StaSSID').value,
			// 'StaPassword'				:	document.getElementById('StaPassword').value,
			// 'StaEnable'					:	(document.getElementById('StaEnable').checked ? 1 : 0)
			// };
	// $.ajax({
        // type        : 'POST',
        // url         : '/config',
        // contentType	: 'application/json; charset=utf-8',
        // data        : JSON.stringify(formData),
        // dataType	: 'json'
    // })
// }
function post_netcfg(event) {
    event.preventDefault();
    var formData = {
            'StaSSID'                    :   document.getElementById('StaSSID').value,
            'StaPassword'                :   document.getElementById('StaPassword').value,
            'StaEnable'                  :   (document.getElementById('StaEnable').checked ? 1 : 0)
            };
    var xhr = new XMLHttpRequest();
    xhr.open('POST', '/config', true);
    xhr.setRequestHeader('Content-Type', 'application/json; charset=utf-8');    
    xhr.send(JSON.stringify(formData));
}

// $( document ).ready(function() {
	// get_config();
// 	
	// document.getElementById('form_netcfg').addEventListener('submit', post_netcfg);
	// document.getElementById('netcfg_cancel').addEventListener('click', get_config);
// });
//Here we put some initial code which starts after DOM loaded
function onDocumentRedy() {
    //Init
    get_config();
    document.getElementById('form_netcfg').addEventListener('submit', post_netcfg);
    document.getElementById('netcfg_cancel').addEventListener('click', get_config);

}

document.addEventListener('DOMContentLoaded', onDocumentRedy);