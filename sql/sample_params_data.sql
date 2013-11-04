PRAGMA foreign_keys=OFF;

BEGIN TRANSACTION;

INSERT INTO "interfaces" VALUES(1,1,100,'ARDUINO01','Compteurs EDF-ERDF','SERIAL://ttyACM0','',1);
INSERT INTO "interfaces" VALUES(2,2,200,'MESHNETWORK01','Reseau de capteurs/actionneurs XBEE','SERIAL://ttyUSB0','',1);
INSERT INTO "interfaces" VALUES(3,3,201,'XBEE01','Interface actionneur VMC','MESH://0013a200-4079a4c0','PLUGIN=xbee_generic;PARAMETERS=#d1:digital_out_h,#d2:analog_in,#d3:digital_in,#set_name:$my_name,#set_sample:5000',2);
INSERT INTO "interfaces" VALUES(4,4,201,'XBEE02','Interface temperature et humidite','MESH://0013a200-4079a4b9','PLUGIN=generic;PARAMETERS=#set_name:$my_name',2);

INSERT INTO "sensors_actuators" VALUES(1,1,1002,1,'R1','Commande telerupteur SALLE1',1,'PIN=AI0;MODE=DIGITAL;ACTION=PULSE',1);
INSERT INTO "sensors_actuators" VALUES(2,2,1001,1,'TEMP01','Temperature du capteur TMP36',1,'PIN=AI3;MODE=ANALOG;COMPUTE=TEMP;ALGO=TMP36',1);
INSERT INTO "sensors_actuators" VALUES(3,3,2000,4,'HUMI01','Capteur d humidite DHT22',1,'PLUGIN=my_dht22;PARAMETERS=NONE',1);
INSERT INTO "sensors_actuators" VALUES(4,4,2001,4,'TEMP01','Capteur de temperature DHT22',1,'PLUGIN=my_dht22;PARAMETERS=NONE',1);
INSERT INTO "sensors_actuators" VALUES(5,5,2002,4,'PILE01','Capteur de niveau des piles DHT22',1,'PLUGIN=my_dht22;PARAMETERS=NONE',1);
INSERT INTO "sensors_actuators" VALUES(10,9,201,3,'R2','RELAI XBEE D1',1,'PLUGIN=vmc;PARAMETERS=D1:4',1);
INSERT INTO "sensors_actuators" VALUES(11,10,201,3,'I1','ENTREE A2 XBEE',1,'PLUGIN=xbee_generic;PARAMETERS=PIN:A2',1);
INSERT INTO "sensors_actuators" VALUES(12,11,201,3,'I2','ENTREE D3 XBEE',1,'PLUGIN=xbee_generic;PARAMETERS=PIN:D3',1);
INSERT INTO "sensors_actuators" VALUES(13,11,1000,1,'CNTR0','COMPTEUR PROD',1,'COUNTER=0',1);

COMMIT;