CREATE DATABASE DOMOTIQUE_DEV;

CREATE USER domotique_dev;
CREATE USER domotique_dev@'localhost';

SET PASSWORD FOR domotique_dev = PASSWORD('maison');
SET PASSWORD FOR domotique_dev@'localhost' = PASSWORD('maison');

GRANT SELECT, INSERT, DELETE, UPDATE, LOCK TABLES ON domotique_dev.* TO 'domotique_dev'@'%' WITH GRANT OPTION;
GRANT SELECT, INSERT, DELETE, UPDATE, LOCK TABLES ON domotique_dev.* TO 'domotique_dev'@'localhost' WITH GRANT OPTION;

FLUSH PRIVILEGES;

USE DOMOTIQUE_DEV;
