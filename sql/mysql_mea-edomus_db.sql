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
   date DATETIME,
   value1 FLOAT(13,3),
   unit SMALLINT UNSIGNED,
   value2 FLOAT(13,3),
   complement VARCHAR(255),

   PRIMARY KEY(id)
);

GRANT SELECT, INSERT, DELETE, UPDATE, LOCK TABLES ON domotique.* TO 'domotique'@'%' WITH GRANT OPTION;
GRANT SELECT, INSERT, DELETE, UPDATE, LOCK TABLES ON domotique.* TO 'domotique'@'localhost' WITH GRANT OPTION;

FLUSH PRIVILEGES;

SHOW COLUMNS FROM sensors_values;
