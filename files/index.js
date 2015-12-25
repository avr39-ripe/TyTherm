$( document ).ready(function() {
	
	(function worker() {
		$.getJSON('/state', function(data) {
			document.getElementById('counter').textContent = data.counter;
			document.getElementById('temperature').innerHTML = data.temperature + ' &deg;C';
			setTimeout(worker, 5000);
		});
	})();
});