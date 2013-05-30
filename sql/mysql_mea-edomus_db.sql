CREATE DATABASE DOMOTIQUE;

CREATE USER domotique;
CREATE USER domotique@'localhost';

SET PASSWORD FOR domotique = PASSWORD('passwd');
SET PASSWORD FOR domotique@'localhost' = PASSWORD('passwd');

GRANT SELECT, INSERT, DELETE, UPDATE, LOCK TABLES ON domotique.* TO 'domotique'@'%' WITH GRANT OPTION;
GRANT SELECT, INSERT, DELETE, UPDATE, LOCK TABLES ON domotique.* TO 'domotique'@'localhost' WITH GRANT OPTION;

FLUSH PRIVILEGES;

USE DOMOTIQUE;


CREATE TABLE IF NOT EXISTS temperatures (
	id INT UNSIGNED NOT NULL AUTO_INCREMENT,
	date DATETIME,
	temp FLOAT,
   id_location TINYINT,
	id_thermometer TINYINT,
	flag TINYINT,
	
	PRIMARY KEY(id)
);

SHOW COLUMNS FROM temperatures;


CREATE TABLE IF NOT EXISTS electricity_counters (
	record_id INT UNSIGNED NOT NULL AUTO_INCREMENT,
   sensor_id TINYINT,
	date DATETIME,
	wh INT UNSIGNED,
	kwh MEDIUMINT UNSIGNED,
	flag TINYINT,
	
	PRIMARY KEY(record_id)
);

SHOW COLUMNS FROM electricity_counters;


CREATE TABLE IF NOT EXISTS pinst (
	record_id INT UNSIGNED NOT NULL AUTO_INCREMENT,
   sensor_id TINYINT,
	date DATETIME,
	power FLOAT,
	delta_t FLOAT,
	flag TINYINT,
	
	PRIMARY KEY(record_id)
);

SHOW COLUMNS FROM pinst;

