<?php
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');
session_start();

$debug=0;

switch(check_admin()){
    case 98:
        break;
    case 99:
        error_log("not connected");
        echo json_encode(array('isError' => true, 'msg' => 'not connected')); // pour edatagrid voir http://www.jeasyui.com/extension/edatagrid.php (onError)
        exit(1);
    case 0:
        break;
    default:
        error_log("unknown error");
        echo json_encode(array('isError' => true, 'msg' => 'unknown'));
        exit(1);
}

//parametres passés si pagination sur la datagrid
$page = isset($_POST['page']) ? intval($_POST['page']) : 1;
$rows = isset($_POST['rows']) ? intval($_POST['rows']) : 10;

// parametres passés si tri sur la datagrid
$sort = isset($_POST['sort'])   ? strval($_POST['sort']) : 'id';
$order = isset($_POST['order']) ? strval($_POST['order']) : 'asc';
$offset = ($page-1)*$rows;


// connexion à la db
try {
    $file_db = new PDO($PARAMS_DB_PATH);
}
catch(PDOException $e) {

    $error_msg=$e->getMessage(); 
    error_log($error_msg);
    echo json_encode(array('isError' => true, 'msg' => $error_msg));
    exit(1);
}

// parametrage du requetage
// $file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
// $file_db->setAttribute(PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

// récupération du nombre total de ligne

try {
   $request = $file_db->query("SELECT count(*) FROM sensors_actuators");
   $row = $request->fetch(PDO::FETCH_NUM);
   $total=$row[0];
}
catch (PDOException $e) {
   $error_msg=$e->getMessage(); 
   error_log($error_msg);
   echo json_encode(array('isError' => true, 'msg' => $error_msg));
   $file_db=null;
   exit(1);
}

// requete des données
$SQL="SELECT sensors_actuators.id AS id,
             sensors_actuators.id_sensor_actuator AS id_sensor_actuator,
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
         ON sensors_actuators.id_location = locations.id_location
      ORDER BY $sort $order
      LIMIT $offset, $rows";

try {
   $request = $file_db->query($SQL);
   $rows = array();
   while($row = $request->fetch(PDO::FETCH_OBJ))
   {
      array_push($rows, $row);
   }
}
catch(PDOException $e)
{
    $error_msg=$e->getMessage(); 
    error_log($error_msg);
    echo json_encode(array('isError' => true, 'msg' => $error_msg));
    $file_db=null;
    exit(1);
}

// pour debug
if ($debug==1)
{
   ob_start();
   print_r($rows);
   $debug_msg = ob_get_contents();
   ob_end_clean();
   error_log($debug_msg);
}

$response = array();
$response["total"]=$total;
$response["rows"]=$rows;

// emission des données
header('Content-type: application/json');
echo json_encode($response);

$file_db = null;

// voir http://stackoverflow.com/questions/12911546/jquery-easy-ui-datagrid-search-not-work
/* voir http://www.jeasyui.com/tutorial/datagrid/datagrid8.php */
