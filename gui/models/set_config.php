<?php
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');
session_start();

$check=check_admin();
switch($check){
    case 98:
        echo json_encode(array("result"=>"KO","error"=>98,"error_msg"=>"pas habilité" ));
        exit(1);
    case 99:
        echo json_encode(array("result"=>"KO","error"=>99,"error_msg"=>"non connecté" ));
        exit(1);
    case 0:
        break;
    default:
        echo json_encode(array("result"=>"KO","error"=>1,"error_msg"=>"erreur inconnue (".$check.")." ));
        exit(1);
}

//$fields=[];
$fields[0]="VENDORID";
$fields[1]="DEVICEID";
$fields[2]="INSTANCEID";
$fields[3]="PLUGINPATH";
$fields[4]="DBSERVER";
$fields[5]="DBPORT";
$fields[6]="DATABASE";
$fields[7]="USER";
$fields[8]="PASSWORD";
$fields[9]="BUFFERDB";

try {
    $file_db = new PDO($PARAMS_DB_PATH);
}catch (PDOException $e){
    echo json_encode(array("error"=>1,"error_msg"=>$e->getMessage() ));
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

for($i=0;$i<10;$i++) {
    $SQL="UPDATE application_parameters set value=\"".$_GET[$fields[$i]]."\" WHERE key=\"".$fields[$i]."\"";
    try{    
        $stmt = $file_db->prepare($SQL);
        $stmt->execute();
    }catch(PDOException $e){
        echo json_encode(array("error"=>2,"error_msg"=>$e->getMessage() ));
        $file_db=null;
        exit(1);
    }
}
echo json_encode(array("retour"=>"OK"));

$file_db = null;
