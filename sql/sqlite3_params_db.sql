
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
input_value TEXT,
nb_conditions INTEGER
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


// rule1 : 1, 1, "rule1", "mea-edomus.home", "sensor.basic", 1, 1, "TRUE" (device = "push1", data1 = "HIGH")
// rule2 : 2, 2, "rule2", "mea-edomus.home", "sensor.basic", 1, 1, "FALSE" (device = "push1", data1 = "LOW")
// rule3 : 3, 3, "rule3", "mea-edomus.home", "sensor.basic", 1, 2, "TRUE" (device = "humi1", unit = "%", data1 > 80)
// rule4 : 4, 4, "rule4", "mea-edomus.home", "sensor.basic", 1, 2, "FALSE" (device = "humi1", unit = "%", data1 <= 50)
// rule5 : 5, 5, "rule5", "mea-edomus.home", "sensor.basic", 1, 3, "TRUE" (device = "XHUMI01", type = "humidity", current > 70)
// rule5 : 6, 6, "rule6", "mea-edomus.home", "sensor.basic", 1, 3, "FALSE" (device = "XHUMI01", type = "humidity", current < 50)

INSERT INTO "rules" VALUES(1,1,"rule1","mea-edomus.home","sensor.basic", 1, 1, "TRUE",  2);
INSERT INTO "rules" VALUES(2,2,"rule2","mea-edomus.home","sensor.basic", 1, 1, "FALSE", 2);
INSERT INTO "rules" VALUES(3,3,"rule3","mea-edomus.home","sensor.basic", 1, 2, "TRUE",  3);
INSERT INTO "rules" VALUES(4,4,"rule4","mea-edomus.home","sensor.basic", 1, 2, "FALSE", 3);
INSERT INTO "rules" VALUES(5,5,"rule5","mea-edomus.home","sensor.basic", 1, 3, "FALSE", 3);
INSERT INTO "rules" VALUES(6,6,"rule6","mea-edomus.home","sensor.basic", 1, 3, "TRUE",  3);



INSERT INTO "conditions" VALUES (1, 1, 1, "C1", "device", "push1", 1);
INSERT INTO "conditions" VALUES (2, 2, 1, "C2", "current", "HIGH", 1);

INSERT INTO "conditions" VALUES (3, 3, 2, "C3", "device", "push1", 1);
INSERT INTO "conditions" VALUES (4, 4, 2, "C4", "current", "LOW", 1);

INSERT INTO "conditions" VALUES (5, 5, 3, "C5", "device", "humi1", 1);
INSERT INTO "conditions" VALUES (6, 6, 3, "C6", "unit", "%", 1);
INSERT INTO "conditions" VALUES (7, 7, 3, "C7", "current", "80", 2);

INSERT INTO "conditions" VALUES (8, 8, 4, "C8", "device", "humi1", 1);
INSERT INTO "conditions" VALUES (9, 9, 4, "C9", "unit", "%", 1);
INSERT INTO "conditions" VALUES (10, 10, 4, "C10", "current", "50", 2);

INSERT INTO "conditions" VALUES (11, 11, 5, "C11", "device", "XHUMI01", 1);
INSERT INTO "conditions" VALUES (12, 12, 5, "C12", "type", "humidity", 1);
INSERT INTO "conditions" VALUES (13, 13, 5, "C13", "current", "70", 5);

INSERT INTO "conditions" VALUES (14, 12, 6, "C14", "device", "XHUMI01", 1);
INSERT INTO "conditions" VALUES (15, 13, 6, "C15", "type", "humidity", 1);
INSERT INTO "conditions" VALUES (16, 14, 6, "C16", "current", "50", 3);

SELECT
   conditions.id_rule,
   rules.name,
   rules.input_type,
   rules.input_index,
   rules.input_value
   FROM conditions
   JOIN rules ON conditions.id_rule = rules.id_rule
      WHERE ((key = 'device' AND value = 'XHUMI01') OR (key = 'current') OR (key = 'type' AND value = 'humidity') OR (key = 'last'))
      AND rules.source="mea-edomus.home"
      AND rules.schema="sensor.basic"
   GROUP BY conditions.id_rule
   HAVING COUNT(conditions.id_rule) = rules.nb_conditions;

select
   count(conditions.id_rule),
   conditions.id_rule
   from conditions
   join rules on conditions.id_rule = rules.id_rule
   where (key='device' AND value="XHUMI01") OR (key="current") OR (key='type' AND value='humidity') OR (key = 'last')
   GROUP BY conditions.id_rule HAVING rules.nb_conditions = count(conditions.id_rule);
   

-- resultat :
-- 1|rule1|1|1|TRUE
-- 2|rule2|1|1|FALSE

SELECT name, key, op, value FROM conditions WHERE id_rule=1 ;
-- resultat :
-- C1|device|1|push1
-- C2|data1|1|HIGH

SELECT name, key, op, value FROM conditions WHERE id_rule=2 ;
-- C3|device|1|push1
-- C4|data1|1|LOW


SELECT
   conditions.id_rule,
   rules.name,
   rules.input_type,
   rules.input_index,
   rules.input_value
   FROM conditions
   JOIN rules ON conditions.id_rule = rules.id_rule
      WHERE ((key="device" AND value="humi1") OR (key="unit" AND value="%") OR (key="data1"))
      AND rules.source="mea-edomus.myhome"
      AND rules.schema="sensor.basic"
   GROUP BY conditions.id_rule
   HAVING COUNT(conditions.id_rule) = rules.nb_conditions;
   
-- resultat :
-- 3|rule3|1|2|TRUE
-- 4|rule4|1|2|FALSE


SELECT
   conditions.id_rule,
   rules.name,
   rules.input_type,
   rules.input_index,
   rules.input_value
   FROM conditions
   JOIN rules ON conditions.id_rule = rules.id_rule
   WHERE ((key="device") OR (key="data1") OR (key="unitE") OR (key="null"))
      AND rules.source="mea-edomus.myhome"
      AND rules.schema="sensor.basic"
   GROUP BY conditions.id_rule
   HAVING COUNT(conditions.id_rule) = rules.nb_conditions

/*
char *get_rules_sql="\
SELECT\
   conditions.id_rule,\
   rules.name,\
   input_type,\
   input_index,\
   input_value\
   FROM conditions\
   JOIN rules ON conditions.id_rule = rules.id_rule\
   WHERE (%s)\
      AND rules.source='%s'\
      AND rules.schema='%s'\
   GROUP BY conditions.id_rule\
   HAVING COUNT(conditions.id_rule) = rules.nb_conditions;";
   
char conditions_keys[1024];
char condition_key[256];

xPL_NameValueListPtr body = xPL_getMessageBody(theMessage);
int n = xPL_getNamedValueCount(body);
for (int16_t i = 0; i < n; i++)
{
   xPL_NameValuePairPtr keyValuePtr = xPL_getNamedValuePairAt(body, i);
   if(!isnumber(keyValuePtr->itemValue)) // pour les valeurs non numérique seul l'égalité est possible, on peut donc améliorer la recherche
      snprintf(condition_key,sizeof(condition_key),"(key = %s AND value = %s), keyValuePtr->itemName, keyValuePtr->itemValue);
   else
      snprintf(condition_key,sizeof(condition_key),"(key = %s), keyValuePtr->itemName);
   if(i)
      strcat(conditions_keys," OR ");
   strcat(condition_keys, condition_key);
   
   // executer la requete SQL
}

snprintf(sql,sizeof(sql),get_rules_sql,conditions_keys,source,schema);
*/
