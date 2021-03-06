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

Prado::using('System.Web.UI.ActiveControls.TActiveCustomValidator');
Prado::using('System.Web.UI.ActiveControls.TActiveDataGrid');
Prado::using('Application.Portlets.Portlets');

class ClientConfiguration extends Portlets {

	public function onInit($param) {
		parent::onInit($param);
		$this->Status->setActionClass($this);
		$this->Apply->setActionClass($this);
	}

	public function configure($clientId) {
		$clientdata = $this->Application->getModule('api')->get(array('clients', 'show', $clientId))->output;
		$this->ShowClient->Text = implode(PHP_EOL, $clientdata);
		$client = $this->Application->getModule('api')->get(array('clients', $clientId))->output;
		$this->ClientName->Text = $client->name;
		$this->ClientIdentifier->Text = $client->clientid;
		$this->ClientDescription->Text = $client->uname;
		$this->FileRetention->Text = intval($client->fileretention / 86400); // conversion to days
		$this->JobRetention->Text = intval($client->jobretention / 86400); // conversion to days
		$this->AutoPrune->Checked = $client->autoprune == 1;

		$jobs_for_client = $this->Application->getModule('api')->get(array('clients', 'jobs', $client->clientid))->output;
		$this->JobsForClient->DataSource = $this->Application->getModule('misc')->objectToArray($jobs_for_client);
		$this->JobsForClient->dataBind();
	}

	public function status($sender, $param) {
		$status = $this->Application->getModule('api')->get(array('clients', 'status', $this->ClientIdentifier->Text))->output;
		$this->ShowClient->Text = implode(PHP_EOL, $status);
	}

	public function apply($sender, $param) {
		if($this->JobRetentionValidator->IsValid === false || $this->FileRetentionValidator->IsValid === false) {
			return false;
		}
		$client = array();
		$client['clientid'] = $this->ClientIdentifier->Text;
		$client['fileretention'] = $this->FileRetention->Text * 86400; // conversion to seconds
		$client['jobretention'] = $this->JobRetention->Text * 86400; // conversion to seconds
		$client['autoprune'] = (integer)$this->AutoPrune->Checked;
		$this->Application->getModule('api')->set(array('clients', $client['clientid']), $client);
	}

	public function fileRetentionValidator($sender, $param) {
		$isValid = preg_match('/^\d+$/', $this->FileRetention->Text) && $this->FileRetention->Text >= 0;
		$param->setIsValid($isValid);
	}

	public function jobRetentionValidator($sender, $param) {
		$isValid = preg_match('/^\d+$/', $this->JobRetention->Text) && $this->JobRetention->Text >= 0;
		$param->setIsValid($isValid);
	}

	public function openJob($sender, $param) {
		$jobid = $param->CallbackParameter;
		$this->getPage()->JobConfiguration->configure($jobid);
	}
}
?>
