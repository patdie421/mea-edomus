<?php
//ini_set('error_reporting', E_ALL & ~E_NOTICE);
ini_set('error_reporting', E_ALL);
ini_set('log_errors', 'On');      // log to file (yes)
ini_set('display_errors', 'Off'); // log to screen (no)
ini_set("error_log", "/Data/mea_log.txt");

$TITRE_APPLICATION='Mea eDomus Admin';

$PARAMS_DB_PATH='sqlite:/Data/mea-edomus/var/db/params.db';
$QUERYDB_SQL='sql/querydb.sql';
