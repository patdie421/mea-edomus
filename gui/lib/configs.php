<?php
ini_set('error_reporting', E_ERROR);
ini_set('log_errors', 'On');
ini_set('display_errors', 'Off');
ini_set('error_log', "/Data/test5/var/log/php.log");
ini_set('session.save_path', "/Data/test5/var/sessions");
$TITRE_APPLICATION='Mea eDomus Admin';
$BASEPATH='/Data/test5';
$PARAMS_DB_PATH='sqlite:/Data/test5/var/db/params.db';
$QUERYDB_SQL='sql/querydb.sql';
$IOSOCKET_PORT=8000;
$LANG='fr';
