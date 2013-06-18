PRAGMA foreign_keys=OFF;

BEGIN TRANSACTION;

INSERT INTO "interfaces" VALUES(1,1,100,'ARDUINO01','Compteurs EDF-ERDF','SERIAL://tty.usbmodem1421','',1);
INSERT INTO "interfaces" VALUES(2,2,200,'MESHNETWORK01','Reseau de capteurs/actionneurs XBEE','SERIAL://tty.usbserial-A4008TBe','',1);
INSERT INTO "interfaces" VALUES(3,3,201,'XBEE01','Interface actionneur VMC','MESH://0013a200-4079a4c0','PLUGIN=generic;PARAMETERS=#d1:digital_out_h,#d2:analog_in,#set_name:$my_name,#set_sample:5000',2);
INSERT INTO "interfaces" VALUES(4,4,201,'XBEE02','Interface temperature et humidite','MESH://0013a200-4079a4b9','',2);

INSERT INTO "types" VALUES(1,100,'INTYP01','Interface de type 01','');
INSERT INTO "types" VALUES(2,200,'INTYP02','Reseau MESH xbee','');
INSERT INTO "types" VALUES(3,201,'XBEECA','Capteurs et actionneurs a interface XBee','');
INSERT INTO "types" VALUES(4,1000,'PWRCTR','Capteur de compteur ERDF','TEST');
INSERT INTO "types" VALUES(5,1001,'ARDINA','Entree analogique','');
INSERT INTO "types" VALUES(6,1002,'ARDOUTD','Sortie logique','');
INSERT INTO "types" VALUES(7,2000,'XDHT22H','Humidité de DTH22','');
INSERT INTO "types" VALUES(8,2001,'XDHT22T','Température de DTH22','');
INSERT INTO "types" VALUES(9,2002,'XDHT22P','Pile de DTH22','');

INSERT INTO "sensors_actuators" VALUES(1,1,1002,1,'R1','Commande telerupteur SALLE1',20,'PIN=AI0;TYPE=DIGITAL_OUT;ACTION=PULSE',1);
INSERT INTO "sensors_actuators" VALUES(2,2,1001,1,'TEMP01','Temperature du capteur TMP36',10,'PIN=AI3;TYPE=ANALOG_IN;COMPUTE=TEMP;ALGO=TMP36',1);
INSERT INTO "sensors_actuators" VALUES(3,3,2000,4,'HUMI01','Capteur d humidite DHT22',50,'PLUGIN=my_dht22;PARAMETERS=NONE',1);
INSERT INTO "sensors_actuators" VALUES(4,4,2001,4,'TEMP01','Capteur de temperature DHT22',50,'PLUGIN=my_dht22;PARAMETERS=NONE',1);
INSERT INTO "sensors_actuators" VALUES(5,5,2002,4,'PILE01','Capteur de niveau des piles DHT22',50,'PLUGIN=my_dht22;PARAMETERS=NONE',1);
INSERT INTO "sensors_actuators" VALUES(10,9,201,3,'RELY01','RELAI XBEE D1',10,'PLUGIN=vmc;PARAMETERS=D1:4',1);
INSERT INTO "sensors_actuators" VALUES(11,10,201,3,'TEST01','ENTREE D2 XBEE',10,'PLUGIN=generic;PARAMETERS=PIN:A2',1);
INSERT INTO "sensors_actuators" VALUES(NULL,11,1000,3,'CNTR0','COMPTEUR PROD',10,'COUNTER=0',1);

INSERT INTO "locations" VALUES(1,10,'TABLEAU','Tableau electrique');
INSERT INTO "locations" VALUES(2,20,'SALON','Salon');
INSERT INTO "locations" VALUES(3,50,'SDB BAS','Salle de bain RDC');

INSERT INTO "application_parameters" VALUES(1, 'BUFFERDB', '/Data/mea-edomus/queries.db', '');
INSERT INTO "application_parameters" VALUES(2, 'DBSERVER', '192.168.0.22', '');
INSERT INTO "application_parameters" VALUES(3, 'DATABASE', 'domotique', '');
INSERT INTO "application_parameters" VALUES(4, 'USER', 'domotique', '');
INSERT INTO "application_parameters" VALUES(5, 'PASSWORD', 'maison', '');
INSERT INTO "application_parameters" VALUES(6, 'VENDORID', 'mea', '');
INSERT INTO "application_parameters" VALUES(7, 'DEVICEID', 'edomus', '');
INSERT INTO "application_parameters" VALUES(8, 'INSTANCEID', 'cheznousdev', '');
INSERT INTO "application_parameters" VALUES(9, 'PLUGINPATH', '/Data/mea-edomus/plugins', '');


COMMIT;
