CREATE DATABASE domotique;

CREATE USER domotique;
CREATE USER domotique@'localhost';

SET PASSWORD FOR domotique = PASSWORD('maison');
SET PASSWORD FOR domotique@'localhost' = PASSWORD('maison');

FLUSH PRIVILEGES;

USE domotique;

CREATE TABLE IF NOT EXISTS sensors_values (
   id INT UNSIGNED NOT NULL AUTO_INCREMENT,
   sensor_id SMALLINT UNSIGNED,
   collector_id SMALLINT UNSIGNED,
   location_id SMALLINT UNSIGNED,
   date DATETIME,
   value1 FLOAT(13,3),
   unit SMALLINT UNSIGNED,
   value2 FLOAT(13,3),
   complement VARCHAR(255),
   archive_flag SMALLINT UNSIGNED,
   PRIMARY KEY(id)
);

DROP INDEX date ON sensors_values ;
DROP INDEX sensor_id ON sensors_values ;
DROP INDEX collector_id ON sensors_values ;

ALTER TABLE sensors_values ADD INDEX (date);
ALTER TABLE sensors_values ADD INDEX (sensor_id);
ALTER TABLE sensors_values ADD INDEX (collector_id);

CREATE TABLE IF NOT EXISTS sensors_names (
   id INT UNSIGNED NOT NULL AUTO_INCREMENT,
   sensor_id SMALLINT UNSIGNED,
   name VARCHAR(40),
   description VARCHAR(255),
   PRIMARY KEY(id)
);

CREATE TABLE IF NOT EXISTS collectors_names (
   id INT UNSIGNED NOT NULL AUTO_INCREMENT,
   collector_id SMALLINT UNSIGNED,
   collector_key BIGINT,
   name VARCHAR(40),
   description VARCHAR(255),
   PRIMARY KEY(id)
);


GRANT SELECT, INSERT, DELETE, UPDATE, LOCK TABLES ON domotique.* TO 'domotique'@'%' WITH GRANT OPTION;
GRANT SELECT, INSERT, DELETE, UPDATE, LOCK TABLES ON domotique.* TO 'domotique'@'localhost' WITH GRANT OPTION;

FLUSH PRIVILEGES;

SHOW COLUMNS FROM sensors_values;
SHOW COLUMNS FROM sensors_names;
SHOW COLUMNS FROM collectors_names;
