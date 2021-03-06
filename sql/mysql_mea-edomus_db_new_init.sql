CREATE DATABASE domotique2;

CREATE USER domotique;
CREATE USER domotique@'localhost';

SET PASSWORD FOR domotique = PASSWORD('maison');
SET PASSWORD FOR domotique@'localhost' = PASSWORD('maison');

FLUSH PRIVILEGES;

USE domotique2;

CREATE TABLE IF NOT EXISTS sensors_values (
   id INT UNSIGNED NOT NULL AUTO_INCREMENT,
   sensor_id SMALLINT UNSIGNED,
   collector_id INT UNSIGNED,
   location_id SMALLINT UNSIGNED,
   date DATETIME,
   value1 FLOAT(13,3),
   unit SMALLINT UNSIGNED,
   value2 FLOAT(13,3),
   complement VARCHAR(255),
   archive_flag SMALLINT UNSIGNED,
   PRIMARY KEY(id)
);

ALTER TABLE sensors_values ADD INDEX (date);
ALTER TABLE sensors_values ADD INDEX (sensor_id);
ALTER TABLE sensors_values ADD INDEX (collector_id);

CREATE TABLE IF NOT EXISTS sensors_names (
   id INT UNSIGNED NOT NULL AUTO_INCREMENT,
   sensor_id SMALLINT UNSIGNED,
   collector_id INT UNSIGNED,
   name VARCHAR(40),
   description VARCHAR(255),
   PRIMARY KEY(id)
);

CREATE TABLE IF NOT EXISTS collectors_names (
   id INT UNSIGNED NOT NULL AUTO_INCREMENT,
   collector_id INT UNSIGNED,
   name VARCHAR(40),
   description VARCHAR(255),
   PRIMARY KEY(id)
);


CREATE TABLE IF NOT EXISTS locations (
   id INT UNSIGNED NOT NULL AUTO_INCREMENT,
   location_id INT UNSIGNED,
   collector_id INT UNSIGNED,
   name VARCHAR(40),
   description VARCHAR(255),
   PRIMARY KEY(id)
);


CREATE TABLE IF NOT EXISTS sensors_values_c (
   id INT UNSIGNED NOT NULL AUTO_INCREMENT,
   sensor_id SMALLINT UNSIGNED,
   collector_id INT UNSIGNED,
   location_id SMALLINT UNSIGNED,
   date DATETIME,
   nb_values SMALLINT UNSIGNED,
   avg1 FLOAT(13,3),
   min1 FLOAT(13,3),
   max1 FLOAT(13,3),
   unit SMALLINT UNSIGNED,
   avg2 FLOAT(13,3),
   min2 FLOAT(13,3),
   max2 FLOAT(13,3),
   PRIMARY KEY(id)
);

GRANT SELECT, INSERT, DELETE, UPDATE, LOCK TABLES ON domotique2.* TO 'domotique'@'%' WITH GRANT OPTION;
GRANT SELECT, INSERT, DELETE, UPDATE, LOCK TABLES ON domotique2.* TO 'domotique'@'localhost' WITH GRANT OPTION;

FLUSH PRIVILEGES;

SHOW COLUMNS FROM sensors_values;
SHOW COLUMNS FROM sensors_names;
SHOW COLUMNS FROM collectors_names;
