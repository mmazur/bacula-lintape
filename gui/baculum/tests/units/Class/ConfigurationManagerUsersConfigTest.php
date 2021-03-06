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

require_once('test_common.php');

/**
 * Test cases to read/write/check Baculum users file (baculum.users).
 *
 * @author Marcin Haba
 */
class ConfigurationManagerUsersConfigTest extends PHPUnit_Framework_TestCase {

	public static $application = null;

	public function setUp() {
		if (self::$application === null) {
			self::$application = new TApplicationTest(BACULUM_ROOT_DIR . 'protected');
			self::$application->run();
		}
		if (file_exists(BACULUM_ROOT_DIR . 'protected/Data')) {
			copy_path(BACULUM_ROOT_DIR . 'protected/Data', BACKUP_FILES_PATH);
			remove_path(BACULUM_ROOT_DIR . 'protected/Data', true);
		}
	}

	public function tearDown() {
		if (file_exists(BACKUP_FILES_PATH)) {
			remove_path(BACKUP_FILES_PATH . 'protected/Data', true);
			copy_path(BACKUP_FILES_PATH, BACULUM_ROOT_DIR . 'protected/Data');
			remove_path(BACKUP_FILES_PATH);
		}
	}

	public function testSetUsersConfig() {
		$testData = array();
		$testData[] = array( // create new user admin432
			'data' => array(
				'user' => 'admin432',
				'password' => 'myStrOnGPa$$!',
				'firstusage' => true,
				'olduser' => null
			),
			'result' => 'admin432:bXOrb7VkxRBYY'
		);
		$testData[] = array( // add user willyWonka to config
			'data' => array(
				'user' => 'willyWonka',
				'password' => '#=ChOcOlAtEE=#',
				'firstusage' => false,
				'olduser' => null
			),
			'result' => "admin432:bXOrb7VkxRBYY\nwillyWonka:IzGy5zT.vOaeI"
		);
		$testData[] = array( // clear users config and create user willyWonka again
			'data' => array(
				'user' => 'willyWonka',
				'password' => '#=ChOcOlAtEE=#',
				'firstusage' => true,
				'olduser' => null
			),
			'result' => "willyWonka:IzGy5zT.vOaeI"
		);
		$testData[] = array( // add user donkamillo to config
			'data' => array(
				'user' => 'donkamillo',
				'password' => '!@#$%^&*()_+}{|"\'<>?/.,',
				'firstusage' => false,
				'olduser' => null
			),
			'result' => "willyWonka:IzGy5zT.vOaeI\ndonkamillo:IU029IHEUYWhY"
		);
		$testData[] = array( // rename user willyWonka to user johny11
			'data' => array(
				'user' => 'johny11',
				'password' => '#=ChOcOlAtEE=#',
				'firstusage' => false,
				'olduser' => 'willyWonka'
			),
			'result' => "donkamillo:IU029IHEUYWhY\njohny11:IzGy5zT.vOaeI"
		);
		$testData[] = array( // change password for user johny11
			'data' => array(
				'user' => 'johny11',
				'password' => 'CoffeeAndTee!@#$%^&*()_+}{|"\'<>?/.:,',
				'firstusage' => false,
				'olduser' => null
			),
			'result' => "donkamillo:IU029IHEUYWhY\njohny11:Q2XfKovUGQe52"
		);
		$testData[] = array( // input the same password again for user johny11
			'data' => array(
				'user' => 'johny11',
				'password' => 'CoffeeAndTee!@#$%^&*()_+}{|"\'<>?/.:,',
				'firstusage' => false,
				'olduser' => null
			),
			'result' => "donkamillo:IU029IHEUYWhY\njohny11:Q2XfKovUGQe52"
		);
		for ($i = 0; $i < count($testData); $i++) {
			$result = self::$application->getModule('configuration')->setUsersConfig(
				$testData[$i]['data']['user'],
				$testData[$i]['data']['password'],
				$testData[$i]['data']['firstusage'],
				$testData[$i]['data']['olduser']
			);
			$this->assertTrue($result);
			$result = file_get_contents(BACULUM_ROOT_DIR . 'protected/Data/baculum.users');
			$this->assertEquals($testData[$i]['result'], $result);
		}
	}

	public function testCheckAndClearUsersConfig() {
		$testData[] = array( // input the same password again for user johny11
			'data' => array(
				'user' => 'johny11',
				'password' => 'CoffeeAndTee!@#$%^&*()_+}{|"\'<>?/.:,',
				'firstusage' => false,
				'olduser' => null
			),
			'result' => "johny11:Q2XfKovUGQe52"
		);
		$result = self::$application->getModule('configuration')->isUsersConfig();
		$this->assertFalse($result);
		for ($i = 0; $i < count($testData); $i++) {
			$result = self::$application->getModule('configuration')->setUsersConfig(
				$testData[$i]['data']['user'],
				$testData[$i]['data']['password'],
				$testData[$i]['data']['firstusage'],
				$testData[$i]['data']['olduser']
			);
			$this->assertTrue($result);
			$result = file_get_contents(BACULUM_ROOT_DIR . 'protected/Data/baculum.users');
			$this->assertEquals($testData[$i]['result'], $result);
		}
		$result = self::$application->getModule('configuration')->isUsersConfig();
		$this->assertTrue($result);
		$result = self::$application->getModule('configuration')->clearUsersConfig();
		$this->assertTrue($result);
	}

	public function testGetAllUsers() {
		$testData = array();
		$testData[] = array(
			'user' => 'hmS2Tbx4VispuvjgRcDPIqadG3ylkn9AELeo1FYQCHWKOZw8N6J5MfBtX7rUz',
			'password' => 'enEzofZKX2/wM'
		);
		$testData[] = array(
			'user' => 'hmS2Tbx4VispuvjgRcDPIqadG3ylkn9AELeo1FYQCHWKOZw8N6J5MfBtX7rUz$',
			'password' => 'xbe9m5cgdFzc2'
		);
		$testData[] = array(
			'user' => 'ąśźćłóżźćń',
			'password' => 'ąśźćłóżźćń'
		);
		$testData[] = array(
			'user' => '0',
			'password' => 'enEzofZK!X2/wM'
		);
		$testData[] = array(
			'user' => '-1-',
			'password' => '!@#$%^&*()_)O(*&^%$#@@!}{PO|":L?><M~><][\';/.,<'
		);
		$testData[] = array(
			'user' => '(myUsEr$^)',
			'password' => ':::::'
		);
		$testData[] = array(
			'user' => 'abcd#$@!',
			'password' => 'MzfDL89YPyQeM'
		);
		$testData[] = array(
			'user' => '^$$#@@!!!:-)',
			'password' => 'PTwuUMPFz/0PE'
		);

		// check if config does not exist
		$result = self::$application->getModule('configuration')->isUsersConfig();
		$this->assertFalse($result);

		// add users to config
		for ($i = 0; $i < count($testData); $i++) {
			$result = self::$application->getModule('configuration')->setUsersConfig(
				$testData[$i]['user'],
				$testData[$i]['password']
			);
			$this->assertTrue($result);
		}

		// read users from config
		$result = self::$application->getModule('configuration')->getAllUsers();

		$this->assertEquals(count($testData), count($result));

		/*
		 * Check if users (and passwords) added to config and users (and passwords) read from
		 * config are the same
		 */
		for ($i = 0; $i < count($result); $i++) {
			$user = $testData[$i]['user'];
			$plain_pwd = $testData[$i]['password'];
			$this->assertTrue(array_key_exists($user, $result));
			$enc_pwd = self::$application->getModule('configuration')->getCryptedPassword($plain_pwd);
			$this->assertEquals($enc_pwd, $result[$testData[$i]['user']]);
		}
	}
}
?>
