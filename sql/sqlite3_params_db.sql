
CREATE TABLE application_parameters
(
id INTEGER PRIMARY KEY,
key TEXT,
value TEXT,
complement TEXT
);


CREATE TABLE interfaces
(
id INTEGER PRIMARY KEY,
id_interface INTEGER,
id_type INTEGER,
name TEXT,
description TEXT,
dev TEXT,
parameters TEXT,
state INTEGER
);


CREATE TABLE types
(
id INTEGER PRIMARY KEY,
id_type INTEGER,
name TEXT,
description TEXT,
parameters TEXT
);


CREATE TABLE sensors_actuators
(
id INTEGER PRIMARY KEY,
id_sensor_actuator INTEGER,
id_type INTEGER,
id_interface INTERGER,
name TEXT,
description TEXT,
id_location INTEGER,
parameters TEXT,
state INTEGER
);


CREATE TABLE locations
(
id INTEGER PRIMARY KEY,
id_location INTEGER,
name TEXT,
description TEXT
);


CREATE TABLE rules
(
id INTEGER PRIMARY KEY,
id_rule INTEGER,
name TEXT,
source TEXT,
schema TEXT,
input_type INTEGER,
input_index INTEGER,
input_value TEXT
);

CREATE TABLE conditions
(
id INTEGER PRIMARY KEY,
id_condition INTEGER,
id_rule INTEGER,
name TEXT,
key TEXT,
value TEXT,
op INTEGER
);

// rule1 : 1, 1, "rule1", "mea-edomus.myhome", "sensor.basic", 1, 1, "TRUE" (device = "push1", data1 = "HIGH")
// rule2 : 2, 2, "rule2", "mea-edomus.myhome", "sensor.basic", 1, 1, "FALSE" (device = "push1", data1 = "LOW")
// rule3 : 3, 3, "rule3", "mea-edomus.myhome", "sensor.basic", 1, 2, "TRUE" (device = "humi1", unit = "%", data1 > 80)

INSERT INTO "rules" VALUES(1,1,"rule1","mea-edomus.myhome","sensor.basic", 1, 1, "TRUE");
INSERT INTO "rules" VALUES(2,2,"rule2","mea-edomus.myhome","sensor.basic", 1, 1, "FALSE");
INSERT INTO "rules" VALUES(3,3,"rule3","mea-edomus.myhome","sensor.basic", 1, 2, "TRUE");

INSERT INTO "conditions" VALUES (1, 1, 1, "C1", "device", "push1", 1);
INSERT INTO "conditions" VALUES (2, 2, 1, "C2", "data1", "HIGH", 1);

INSERT INTO "conditions" VALUES (3, 3, 2, "C3", "device", "push1", 1);
INSERT INTO "conditions" VALUES (4, 4, 2, "C4", "data1", "LOW", 1);

INSERT INTO "conditions" VALUES (5, 5, 3, "C5", "device", "humi1", 1);
INSERT INTO "conditions" VALUES (6, 6, 3, "C6", "unit", "%", 1);
INSERT INTO "conditions" VALUES (7, 7, 3, "C7", "data1", "80", 2);


SELECT id_rules FROM
   (SELECT id_rules,COUNT(id_rules) AS C
      FROM conditions
      JOIN rules ON id_rules = rules.id_rule
      WHERE    (key="device")
            OR (key="unit")
            OR (key="data1")
      GROUP BY id_rules
    )
    WHERE c=3 ;

-- resultat : 3

SELECT key, op, value,id_condition FROM conditions WHERE id_rule=3 and op<>1 ;

-- resultat :
--
-- device|1|humi1|5
-- unit|1|%|6
-- data1|2|80|7


SELECT id_rules FROM
   (SELECT id_rules,COUNT(id_rules) AS C
      FROM conditions
      JOIN rules ON id_rules = rules.id_rule
      WHERE    (key="device")
            OR (key="data1")
      GROUP BY id_rules
    )
    WHERE c=2 ;

