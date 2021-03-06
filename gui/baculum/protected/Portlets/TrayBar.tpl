<div id="tray_bar">
	<img src="<%=$this->getPage()->getTheme()->getBaseUrl()%>/gearwheel-icon.png" alt="<%[ Running jobs: ]%>" /> <%[ Running jobs: ]%> <span class="bold" id="running_jobs"></span>
	<img src="<%=$this->getPage()->getTheme()->getBaseUrl()%>/check-icon.png" alt="<%[ Finished jobs: ]%>" /> <%[ Finished jobs: ]%> <span class="bold" id="finished_jobs"></span>
</div>
<script type="text/javascript">
	var oMonitor;
	var default_refresh_interval = 60000;
	var default_fast_refresh_interval = 10000;
	var timeout_handler;
	var last_callback_time = 0;
	var callback_time_offset = 0;
	var oData;
	document.observe("dom:loaded", function() {
		oMonitor = function() {
			return new Ajax.Request('<%=$this->Service->constructUrl("Monitor")%>', {
				onCreate: function() {
					if (job_callback_duration) {
						callback_time_offset = job_callback_duration - last_callback_time;
					}
					last_callback_time = new Date().getTime();
				},
				onSuccess: function(response) {
					if (timeout_handler) {
						clearTimeout(timeout_handler);
					}
					oData = (response.responseText).evalJSON();
					Statistics.grab_statistics(oData, oJobsStates);

					if (PanelWindow.currentWindowId === 'dashboard') {
						Dashboard.update_all(Statistics, TEXT);
					}

					if (oData.running_jobs.length > 0) {
						refreshInterval =  callback_time_offset + default_fast_refresh_interval;
					} else {
						refreshInterval = default_refresh_interval;
					}
					job_callback_func();
					status_callback_func();
					$('running_jobs').update(oData.running_jobs.length);
					$('finished_jobs').update(oData.terminated_jobs.length);
					timeout_handler = setTimeout("oMonitor()", refreshInterval);
 				}
			});
		};
		oMonitor();
	});
	</script>
