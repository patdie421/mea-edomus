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

CREATE TABLE location
(
id INTEGER PRIMARY KEY,
id_location INTEGER,
name TEXT
);

CREATE TABLE sensors_actuators
(
id INTEGER PRIMARY KEY,
id_sensor_acutator INTEGER,
id_type INTEGER,
id_interface INTERGER,
name TEXT,
description TEXT,
id_location INTEGER,
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

