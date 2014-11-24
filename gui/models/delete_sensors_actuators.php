<?php
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');
session_start();

ob_start();
print_r($_REQUEST);
$debug_msg = ob_get_contents();
ob_end_clean();
error_log($debug_msg);

switch(check_admin()){
    case 98:
        break;
    case 99:
        error_log("not connected");
        echo json_encode(array('isError' => true, 'msgErrog' => 'not connected')); // pour edatagrid voir http://www.jeasyui.com/extension/edatagrid.php (onError)
        exit(1);
    case 0:
        break;
    default:
        error_log("unknown error");
        echo json_encode(array('isError' => true, 'msgError' => 'unknown'));
        exit(1);
}

if(!isset($_REQUEST['id']))
{
   echo json_encode(array('isError' => true, 'msgError' => 'id is mandatory'));
   exit(1);
}

$id = $_REQUEST['id'];

// connexion à la db
try {
    $file_db = new PDO($PARAMS_DB_PATH);
}
catch(PDOException $e) {
    $error_msg=$e->getMessage(); 
    error_log($error_msg);
    echo json_encode(array('isError' => true, 'msgError' => $error_msg));
    exit(1);
}
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

$SQL= "DELETE FROM sensors_actuators WHERE id='$id'";

try {
   $request = $file_db->query($SQL);
}
catch (PDOException $e) {
   $error_msg=$e->getMessage(); 
   error_log($error_msg);
   echo json_encode(array('isError' => true, 'msgError' => $error_msg));
   $file_db=null;
   exit(1);
}

echo json_encode(array('isError' => false, 'msgError' => 'done'));

$file_db=null;
