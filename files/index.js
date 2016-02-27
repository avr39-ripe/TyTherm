$( document ).ready(function() {
	
	(function worker() {
		$.getJSON('/state', function(data) {
			document.getElementById('counter').textContent = data.counter;
			document.getElementById('temperature').innerHTML = data.temperature + ' &deg;C';
			var tempPanel = document.getElementById('temperature-panel');
			if (data.healthy) {
				tempPanel.classList.remove("panel-danger");
				tempPanel.classList.add("panel-default");
			}
			else {
				tempPanel.classList.remove("panel-default");
				tempPanel.classList.add("panel-danger");
			}
			setTimeout(worker, 5000);
		});
	})();
});