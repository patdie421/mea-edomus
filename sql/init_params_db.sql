BEGIN TRANSACTION;

INSERT INTO "application_parameters" VALUES(NULL,'BUFFERDB','/data/prod/mea-edomus/var/db/queries.db','');
INSERT INTO "application_parameters" VALUES(NULL,'DBSERVER','192.168.0.22','');
INSERT INTO "application_parameters" VALUES(NULL,'DATABASE','domotique','');
INSERT INTO "application_parameters" VALUES(NULL,'USER','domotique','');
INSERT INTO "application_parameters" VALUES(NULL,'PASSWORD','maison','');
INSERT INTO "application_parameters" VALUES(NULL,'VENDORID','mea','');
INSERT INTO "application_parameters" VALUES(NULL,'DEVICEID','edomus','');
INSERT INTO "application_parameters" VALUES(NULL,'INSTANCEID','cheznousdev','');
INSERT INTO "application_parameters" VALUES(NULL,'PLUGINPATH','/data/prod/mea-edomus/lib/plugins','');

INSERT INTO "types" VALUES(NULL,100,'INTYP01','Interface de type 01','');
INSERT INTO "types" VALUES(NULL,200,'INTYP02','Interface de type 02','');
INSERT INTO "types" VALUES(NULL,1000,'PWRCTR','Capteur de compteur ERDF','TEST');
INSERT INTO "types" VALUES(NULL,1001,'ARDINA','Entree interface type 01','');
INSERT INTO "types" VALUES(NULL,1002,'ARDOUTD','Sortie interface type 02','');

COMMIT;
