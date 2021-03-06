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
 
class Storage extends BaculumAPI {
	public function get() {
		$storageid = intval($this->Request['id']);
		$storage = $this->getModule('storage')->getStorageById($storageid);
		$allowedStorages = $this->getModule('bconsole')->bconsoleCommand($this->director, array('.storage'), $this->user);
		if ($allowedStorages->exitcode === 0) {
			if(!is_null($storage) && in_array($storage->name, $allowedStorages->output)) {
				$this->output = $storage;
				$this->error =  StorageError::ERROR_NO_ERRORS;
			} else {
				$this->output = StorageError::MSG_ERROR_STORAGE_DOES_NOT_EXISTS;
				$this->error = StorageError::ERROR_STORAGE_DOES_NOT_EXISTS;
			}
		} else {
			$this->output = $allowedStorages->output;
			$this->error = $allowedStorages->exitcode;
		}
	}
}

?>
