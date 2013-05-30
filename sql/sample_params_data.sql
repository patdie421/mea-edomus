INSERT INTO "interfaces" VALUES(1,1,1,'ARDUINO01','Compteurs EDF-ERDF','SERIAL://ttyACM0','',1);

INSERT INTO "types" VALUES(1,1,'INTERFACE001','Compteurs_edf','');

INSERT INTO "sensors_actuators" VALUES(1,1,2,1,'PROD','compteur production',0,'M1=0;M2=1;M3=2;M4=3;TRAP=1',1);
INSERT INTO "sensors_actuators" VALUES(2,2,2,1,'CONSO','compteur consommation',0,'M1=10;M2=11;M3=12;M4=13;TRAP=2',1);
INSERT INTO "sensors_actuators" VALUES(4,3,400,1,'RELAY1','RELAY1',1,'PIN=AI0;TYPE=PULSE;PULSEWIDTH=200',1);
INSERT INTO "sensors_actuators" VALUES(5,4,400,1,'RELAY2','RELAY2',1,'PIN=AI1;TYPE=PULSE;PULSEWIDTH=200',1);
INSERT INTO "sensors_actuators" VALUES(6,5,400,1,'RELAY3','RELAY3',1,'PIN=AI2;TYPE=PULSE;PULSEWIDTH=200',1);
INSERT INTO "sensors_actuators" VALUES(7,6,401,1,'TEMP','TEMP',0,'PIN=AI3;TYPE=ANALOG_IN;COMPUTE=TMP36',1);

INSERT INTO "sensors_actuators" VALUES(8,7,400,1,'LED1','LED1',0,'PIN=D11;TYPE=PULSE;PULSEWIDTH=1000',1);
INSERT INTO "sensors_actuators" VALUES(9,8,401,1,'P1','P1',0,'PIN=10;TYPE=DIGITAL_IN',1);

INSERT INTO "location" VALUES(1,0,'UNE PIECE');

INSERT INTO "sensors_actuators" VALUES(10,9,201,3,'RELY01','RELAI XBEE D1',10,'PLUGIN=vmc;PARAMETERS=D1:4',1);

select sensors_actuators.name, group_concat(types.name), sensors_actuators.id_type, interfaces.id_type from sensors_actuators inner join interfaces on sensors_actuators.id_interface=interfaces.id_interface inner join types on interfaces.id_type = types.id_type or sensors_actuators.id_type = types.id_type where interfaces.dev like "MESH%" group by sensors_actuators.name ;

select sensors_actuators.name, sensors_actuators.id_type, types.name, (select types.name from types where interfaces.id_type = types.id_type) from sensors_actuators inner join interfaces on sensors_actuators.id_interface=interfaces.id_interface inner join types on interfaces.id_type = types.id_type or sensors_actuators.id_type = types.id_type where interfaces.dev like "MESH%" group by sensors_actuators.name ;

select sensors_actuators.name, \
       sensors_actuators.id_type, \
       types.name, \
       (select types.name from types where interfaces.id_type = types.id_type) \
from sensors_actuators \
inner join interfaces \
      on sensors_actuators.id_interface=interfaces.id_interface \
inner join types \
      on sensors_actuators.id_type = types.id_type

select sensors_actuators.name,
       sensors_actuators.id_type,
       types.name,
       (select types.name from types where types.id_type = interfaces.id_type)
from sensors_actuators
inner join interfaces
      on sensors_actuators.id_interface=interfaces.id_interface
inner join types
      on sensors_actuators.id_type = types.id_type
where interfaces.dev like "MESH://%" ;

97 5 0 13 a2 0 40 8a d2 c2 81 bf 44 30 0
