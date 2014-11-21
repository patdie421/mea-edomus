<?php
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');
session_start();


switch(check_admin()){
    case 98:
        break;
    case 99:
        echo json_encode(array("result"=>"KO","error"=>99,"error_msg"=>"non connecté" ));
        exit(1);
    case 0:
        break;
    default:
        echo json_encode(array("result"=>"KO","error"=>1,"error_msg"=>"erreur inconnue" ));
        exit(1);
}


try{
    $file_db = new PDO($PARAMS_DB_PATH);
}catch(PDOException $e){
    echo json_encode(array("result"=>"KO","error"=>2,"error_msg"=>$e->getMessage() ));
    exit(1);
}


$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT


$SQL="SELECT sensors_actuators.id AS id,
             sensors_actuators.id_sensor_actuator AS id_sensors_actuators,
             sensors_actuators.id_type AS id_type,
             sensors_actuators.name AS name,
             sensors_actuators.description AS description,
             interfaces.id_interface AS id_interface,
             sensors_actuators.id_location AS id_location,
             sensors_actuators.state AS state,
             sensors_actuators.todbflag AS todbflag,
             sensors_actuators.parameters AS parameters,
             locations.name AS lname,
             interfaces.name AS iname,
             types.name AS tname
      FROM sensors_actuators
      INNER JOIN interfaces
         ON sensors_actuators.id_interface = interfaces.id_interface
      INNER JOIN types
         ON sensors_actuators.id_type = types.id_type
      INNER JOIN locations
         ON sensors_actuators.id_location = locations.id_location";

try {
    $stmt = $file_db->prepare($SQL);
    $stmt->execute();
    $result = $stmt->fetchAll();
}
catch(PDOException $e) {
    echo json_encode(array("result"=>"KO","error"=>4,"error_msg"=>$e->getMessage() ));
    $file_db=null;
    exit(1);
}

header('Content-type: application/json');

$response = array();

foreach ($result as $result_elem){
    array_push($response, $result_elm);
}

echo json_encode($response);

$file_db = null;