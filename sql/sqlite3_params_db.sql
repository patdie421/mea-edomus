
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
