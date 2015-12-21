function get_config() {
	$.getJSON('/config.json',
			function(data) {
				$.each(data, function(key, value){
            		document.getElementById(key).value = value;
            	if (data.sta_enable == 1) {
            		document.getElementById('sta_enable').checked = true;
            	}
            	else
            		document.getElementById('sta_enable').checked = false;
        		});
            });
}


function post_netcfg(event) {
	event.preventDefault();
	var formData = {
			'SSID'					:	document.getElementById('SSID').value,
			'Password'				:	document.getElementById('Password').value,
			'sta_enable'			:	(document.getElementById('sta_enable').checked ? 1 : 0)
			};
	$.ajax({
        type        : 'POST',
        url         : '/config',
        contentType	: 'application/json; charset=utf-8',
        data        : JSON.stringify(formData),
        dataType	: 'json'
    })
}


$( document ).ready(function() {
	get_config();
	
	document.getElementById('form_netcfg').addEventListener('submit', post_netcfg);
	document.getElementById('netcfg_cancel').addEventListener('click', get_config);
});