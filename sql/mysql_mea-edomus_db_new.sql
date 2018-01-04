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

DROP INDEX date ON sensors_values ;
DROP INDEX sensor_id ON sensors_values ;
DROP INDEX collector_id ON sensors_values ;

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

/* export/import
select null as id, sensor_id, collector_id, name, description from sensors_names INTO OUTFILE '/tmp/export2' FIELDS TERMINATED BY ',' OPTIONALLY ENCLOSED BY '"' LINES TERMINATED BY '\n';

SELECT null as id, sensor_id, collector_id, location_id, date, value1, unit, value2 complement, archive_flag
FROM sensors_values
INTO OUTFILE '/tmp/sensors_values.export'
FIELDS TERMINATED BY ','
OPTIONALLY ENCLOSED BY '"'
LINES TERMINATED BY '\n';

Export de la base ancien format

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

Ancienne table :
+------------+----------------------+------+-----+---------+----------------+
| Field      | Type                 | Null | Key | Default | Extra          |
+------------+----------------------+------+-----+---------+----------------+
| id         | int(10) unsigned     | NO   | PRI | NULL    | auto_increment |
| sensor_id  | smallint(5) unsigned | YES  | MUL | NULL    |                |
| date       | datetime             | YES  | MUL | NULL    |                |
| value1     | float(13,3)          | YES  |     | NULL    |                |
| unit       | smallint(5) unsigned | YES  |     | NULL    |                |
| value2     | float(13,3)          | YES  |     | NULL    |                |
| complement | varchar(255)         | YES  |     | NULL    |                |
+------------+----------------------+------+-----+---------+----------------+

Nouvelle table :
+--------------+----------------------+------+-----+---------+----------------+
| Field        | Type                 | Null | Key | Default | Extra          |
+--------------+----------------------+------+-----+---------+----------------+
| id           | int(10) unsigned     | NO   | PRI | NULL    | auto_increment |
| sensor_id    | smallint(5) unsigned | YES  | MUL | NULL    |                |
| date         | datetime             | YES  | MUL | NULL    |                |
| value1       | float(13,3)          | YES  |     | NULL    |                |
| unit         | smallint(5) unsigned | YES  |     | NULL    |                |
| value2       | float(13,3)          | YES  |     | NULL    |                |
| complement   | varchar(255)         | YES  |     | NULL    |                |
| collector_id | int(10) unsigned     | YES  | MUL | NULL    |                |
| location_id  | int(11)              | YES  |     | NULL    |                |
| archive_flag | smallint(5) unsigned | YES  |     | NULL    |                |
+--------------+----------------------+------+-----+---------+----------------+

SELECT null as id, sensor_id, date, value1, unit, value2 complement, 0 as collector_id, 0 as location_id, 0 as archive_flag
FROM sensors_values
INTO OUTFILE '/tmp/sensors_values.export'
FIELDS TERMINATED BY ','
OPTIONALLY ENCLOSED BY '"'
LINES TERMINATED BY '\n';
*/
