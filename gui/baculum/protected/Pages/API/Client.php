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
 
class Client extends BaculumAPI {

	public function get() {
		$clientid = intval($this->Request['id']);
		$client = $this->getModule('client')->getClientById($clientid);
		$allowedClients = $this->getModule('bconsole')->bconsoleCommand($this->director, array('.client'), $this->user);
		if ($allowedClients->exitcode === 0) {
			if(!is_null($client) && in_array($client->name, $allowedClients->output)) {
				$this->output = $client;
				$this->error = ClientError::ERROR_NO_ERRORS;
			} else {
				$this->output = ClientError::MSG_ERROR_CLIENT_DOES_NOT_EXISTS;
				$this->error =ClientError::ERROR_CLIENT_DOES_NOT_EXISTS;
			}
		} else {
			$this->output = $allowedClients->output;
			$this->error = $allowedClients->exitcode;
		}
	}

	public function set($id, $params) {
		$result = ($this->user === null) ? $this->getModule('client')->setClient($id, $params) : true;
		if($result === true) {
			$this->output = null;
			$this->error = ClientError::ERROR_NO_ERRORS;
		} else {
			$this->output = DatabaseError::MSG_ERROR_WRITE_TO_DB_PROBLEM;
			$this->error = DatabaseError::ERROR_WRITE_TO_DB_PROBLEM;
		}
	}
}

?>
