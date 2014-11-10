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

$page = $_GET['page']; // get the requested page
$limit = $_GET['rows']; // get how many rows we want to have into the grid
$sidx = $_GET['sidx']; // get index row - i.e. user click to sort
$sord = $_GET['sord']; // get the direction

if(!$sidx)
    $sidx =1;

try{
    $file_db = new PDO($PARAMS_DB_PATH);
}catch(PDOException $e){
    echo json_encode(array("result"=>"KO","error"=>2,"error_msg"=>$e->getMessage() ));
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT
    
try{    
    $stmt = $file_db->prepare("SELECT count(*) AS count FROM (SELECT * FROM sensors_actuators)");
    $stmt->execute();
    $result = $stmt->fetchAll();
}catch(PDOException $e){
    echo json_encode(array("result"=>"KO","error"=>3,"error_msg"=>$e->getMessage() ));
    $file_db=null;
    exit(1);
}
$count=$result[0]['count'];

if( $count >0 ) {
	$total_pages = ceil($count/$limit);
} else {
	$total_pages = 0;
}
if ($page > $total_pages) $page=$total_pages;
$start = $limit*$page - $limit; // do not put $limit*($page - 1)

$SQL="SELECT sensors_actuators.id AS id,
             sensors_actuators.id_sensor_actuator AS id_sensors_actuators,
             sensors_actuators.id_type AS id_type,
             sensors_actuators.name AS name,
             sensors_actuators.description AS description,
             interfaces.id_interface AS id_interface,
             sensors_actuators.id_location AS id_location,
             sensors_actuators.state AS state,".
//            "sensors_actuators.todbflag AS todbflag,".
            "sensors_actuators.parameters AS parameters,
             locations.name AS lname,
             interfaces.name AS iname,
             types.name AS tname
      FROM sensors_actuators
      INNER JOIN interfaces
         ON sensors_actuators.id_interface = interfaces.id_interface
      INNER JOIN types
         ON sensors_actuators.id_type = types.id_type
      INNER JOIN locations
         ON sensors_actuators.id_location = locations.id_location
      ORDER BY $sidx $sord LIMIT $start , $limit";

try{    
    $stmt = $file_db->prepare($SQL);
    $stmt->execute();
    $result = $stmt->fetchAll();
}catch(PDOException $e){
    echo json_encode(array("result"=>"KO","error"=>4,"error_msg"=>$e->getMessage() ));
    $file_db=null;
    exit(1);
}

if ( stristr($_SERVER["HTTP_ACCEPT"],"application/xhtml+xml") ) {
    header("Content-type: application/xhtml+xml;charset=utf-8");
}else{
    header("Content-type: text/xml;charset=utf-8");
}
$et = ">";

echo "<?xml version='1.0' encoding='utf-8'?$et\n";
echo "<rows>";
echo "<page>".$page."</page>";
echo "<total>".$total_pages."</total>";
echo "<records>".$count."</records>";


foreach ($result as $result_elem){
	echo "<row id='".$result_elem['id']."'>";
    echo "<cell>".$result_elem['id']."</cell>";
    echo "<cell>".$result_elem['id_sensors_actuators']."</cell>";
    echo "<cell><![CDATA[".$result_elem['name']."]]></cell>";
    echo "<cell><![CDATA[".$result_elem['tname']."]]></cell>";
    echo "<cell><![CDATA[".$result_elem['description']."]]></cell>";
    echo "<cell><![CDATA[".$result_elem['iname']."]]></cell>";
    echo "<cell><![CDATA[".$result_elem['parameters']."]]></cell>";
    echo "<cell><![CDATA[".$result_elem['lname']."]]></cell>";
    echo "<cell>".$result_elem['state']."</cell>";
  //    echo "<cell>".$result_elem['todbflag']."</cell>";
    echo "<cell>".$result_elem['id_type']."</cell>";
    echo "<cell>".$result_elem['id_interface']."</cell>";
    echo "<cell>".$result_elem['id_location']."</cell>";
    echo "</row>";
}
echo "</rows>";

$file_db = null;
