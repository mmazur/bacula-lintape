<?php
/*
 * Bacula(R) - The Network Backup Solution
 * Baculum   - Bacula web interface
 *
 * Copyright (C) 2013-2016 Kern Sibbald
 *
 * The main author of Baculum is Marcin Haba.
 * The original author of Bacula is Kern Sibbald, with contributions
 * from many others, a complete list can be found in the file AUTHORS.
 *
 * You may use this file and others of this release according to the
 * license defined in the LICENSE file, which includes the Affero General
 * Public License, v3.0 ("AGPLv3") and some additional permissions and
 * terms pursuant to its AGPLv3 Section 7.
 *
 * This notice must be preserved when any source code is
 * conveyed and/or propagated.
 *
 * Bacula(R) is a registered trademark of Kern Sibbald.
 */
 
class Job extends BaculumAPI {
	public function get() {
		$jobid = intval($this->Request['id']);
		$job = $this->getModule('job')->getJobById($jobid);
		$allowedJobs = $this->getModule('bconsole')->bconsoleCommand($this->director, array('.jobs'), $this->user);
		if ($allowedJobs->exitcode === 0) {
			if(!is_null($job) && in_array($job->name, $allowedJobs->output)) {
				$this->output = $job;
				$this->error = JobError::ERROR_NO_ERRORS;
			} else {
				$this->output = JobError::MSG_ERROR_JOB_DOES_NOT_EXISTS;
				$this->error = JobError::ERROR_JOB_DOES_NOT_EXISTS;
			}
		} else {
			$this->output = $allowedJobs->output;
			$this->error = $allowedJobs->exitcode;
		}
	}

	public function remove($id) {
		$jobid = intval($id);
		$job = $this->getModule('job')->getJobById($jobid);
		if(!is_null($job)) {
			$delete = $this->getModule('bconsole')->bconsoleCommand($this->director, array('delete', 'jobid="' . $job->jobid . '"'), $this->user);
			$this->output = $delete->output;
			$this->error = (integer)$delete->exitcode;
		} else {
			$this->output = JobError::MSG_ERROR_JOB_DOES_NOT_EXISTS;
			$this->error = JobError::ERROR_JOB_DOES_NOT_EXISTS;
		}
	}
}

?>
