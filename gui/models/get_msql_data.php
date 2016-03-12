<?php
/**
 * @file   check_mysql_connexion.php
 * @date   2014/11/17
 * @author Patrice Dietsch <patrice.dietsch@gmail.com>
 * @brief  test la connexion à une base mysql.
 * @detail ceci est un cgi de type GET. le va tenter une connexion à une base mysql
 *         avec les parametres server, port, base, user et password. Si la connexion
  *        peut être établie il retourne "OK" sinon "KO" dans un json :  
 *         { result=>["OK" ou "KO"] : OK = connexion possible, KO une erreur
 *           error=>[numéro d'erreur] : les numéros d'erreur possible sont :
 *              1 : erreur inconnue ou erreur de parametre (voir error_msg)
 *             98 : demandeur pas habilité
 *             99 : demandeur pas connecté
 *              2 : erreur de connexion à la base (message db dans error_msg)
 *           errmsg=>[un message],
 *           debug=>[requete sql executée] }
 
 * @param  server  nom ou adresse du serveur mysql
 * @param  port    port d'ecoute du server
 * @param  base    nom de la base à tester
 * @param  user    compte d'accès
 * @param  passord mot de passe du compte
 */
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');

session_start();

if(isset($DEBUG_ON) && ($DEBUG_ON == 1))
{
   ob_start();
   print_r($_REQUEST);
   $debug_msg = ob_get_contents();
   ob_end_clean();
   error_log($debug_msg);
}

$server="127.0.0.1";
$port="3306";
$base="domotique";
$user="domotique";
$password="maison";
$id=6;

/*
$callback="callback";
$_start = (time() - (7 * 24 * 60 * 60))*1000;
$_end = (time()) * 1000;
$start="".$_start."";
$end="".$_end."";
*/

/*
$check=check_admin();
error_log($check);
switch($check){
    case 98:
        break;
    case 99:
        echo json_encode(array('iserror' => true, 'errno'=> 98, 'errorMsg' => 'not connected', "result"=>"KO", "error"=>98, "error_msg"=>"pas habilité" ));
        exit(1);
    case 0:
        break;
    default:
        echo json_encode(array('iserror' => true, 'errno'=> 100, 'errorMsg' => 'unknow error', "result"=>"KO", "error"=>100, "error_msg"=>"erreur inconnue (".$check.")." ));
        exit(1);
}

header('Content-type: application/json; charset=utf-8' );
*/

if(!isset($_REQUEST['sensor_id']))
{
   error_log("sensor_id is mandarory");
   exit(1);
}
else
{
    $sensor_id=$_REQUEST['sensor_id'];
    error_log("sensor_id=".$sensor_id);
}

header('Content-Type: text/javascript');

$callback = $_GET['callback']; 
if (!preg_match('/^[a-zA-Z0-9_]+$/', $callback)) { 
   die('Invalid callback name'); 
} 
 
$start = @$_GET['start']; 
if ($start && !preg_match('/^[0-9]+$/', $start)) { 
   die("Invalid start parameter: $start"); 
} 
 
$end = @$_GET['end']; 
if ($end && !preg_match('/^[0-9]+$/', $end)) { 
   die("Invalid end parameter: $end"); 
} 

/*
$_start = (time() - (365 * 24 * 60 * 60))*1000;
$_end = (time())*1000;
$start="".$_start."";
$end="".$_end."";
*/

if (!$end) $end = time() * 1000; 

$range = $end - $start; 
$startTime = gmstrftime('%Y-%m-%d %H:%M:%S', $start / 1000); 
$endTime = gmstrftime('%Y-%m-%d %H:%M:%S', $end / 1000); 

error_log("start=".$start." end=".$end." range=".$range);
error_log("startTime=".$startTime." endTime=".$endTime);
//         (UNIX_TIMESTAMP( LAST_DAY(DATE_FORMAT(date, \"%Y-%m-01\")) ))*1000 AS tms
//         (UNIX_TIMESTAMP(DATE_FORMAT(date, \"%Y-%m-%d %H:00:00\")))*1000 AS tms
//         (UNIX_TIMESTAMP(date) DIV 900*900)*1000 AS tms
/*
$sql="SELECT
         sensor_id AS id,
         count(sensor_id) AS nb,
         avg(value1) AS avg,
         min(value1) AS min,
         max(value1) AS max,
         min(date) AS date,
         UNIX_TIMESTAMP( max(date) )*1000 AS tms
      FROM sensors_values
      WHERE sensor_id=".$sensor_id." AND date between '$startTime' AND '$endTime'
      GROUP BY sensor_id, DATE_FORMAT(date, \"%Y-%m\")
      LIMIT 0, 5000;"; // mois
*/
if ($range < 5 * 24 * 3600 * 1000) // 15 minutes
{ 
   error_log("15 mn");
   $sql="SELECT
         sensor_id AS id,
         count(sensor_id) AS nb,
         avg(value1) AS avg,
         min(value1) AS min,
         max(value1) AS max,
         min(date) AS date,
         UNIX_TIMESTAMP( max(date) )*1000 AS tms
      FROM sensors_values
      WHERE sensor_id=".$sensor_id." AND date between '$startTime' AND '$endTime'
      GROUP BY sensor_id, UNIX_TIMESTAMP(date) DIV 900
      ORDER BY tms
      LIMIT 0, 5000;";
}
elseif ($range < 31 * 24 * 3600 * 1000) // heure
{ 
   error_log("1 heure");
   $sql="SELECT
         sensor_id AS id,
         count(sensor_id) AS nb,
         avg(value1) AS avg,
         min(value1) AS min,
         max(value1) AS max,
         min(date) AS date,
         (UNIX_TIMESTAMP(max(date)))*1000 AS tms
      FROM sensors_values
      WHERE sensor_id=".$sensor_id." AND date between '$startTime' AND '$endTime'
      GROUP BY sensor_id, DATE_FORMAT(date, \"%Y-%m-%d %H\")
      ORDER BY tms
      LIMIT 0, 5000;";
}
//elseif ($range < 15 * 31 * 24 * 3600 * 1000) // jour
else // jour
{ 
   error_log("1 jour");
   $sql="SELECT
         sensor_id AS id,
         count(sensor_id) AS nb,
         avg(value1) AS avg,
         min(value1) AS min,
         max(value1) AS max,
         min(date) AS date,
         UNIX_TIMESTAMP(max(date))*1000 AS tms
      FROM sensors_values
      WHERE sensor_id=".$sensor_id." AND date between '$startTime' AND '$endTime'
      GROUP BY sensor_id, date(date)
      ORDER BY tms
      LIMIT 0, 5000;";
}


$dns = 'mysql:host='.$server.';dbname='.$base.";port=".$port;
try{
    $db = new PDO(
        $dns,
        $user,
        $password, 
        array(
            PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
            PDO::ATTR_TIMEOUT => 3
        )
    );
} catch(PDOException $e){
//    echo json_encode(array('iserror' => true, 'errno'=> 2, 'errorMsg' => $e->getMessage(), "result"=>"KO", "error"=>2, "error_msg"=>$e->getMessage() ));
    die($e->getMessage());
}
/*
$sql="SELECT
         sensor_id AS id,
         count(sensor_id) AS nb,
         avg(value1) AS avg,
         min(value1) AS min,
         max(value1) AS max,
         min(date) AS date,
         (UNIX_TIMESTAMP(date) DIV 900*900)*1000 AS tms
      FROM sensors_values
      WHERE sensor_id=".$id." AND date between '$startTime' AND '$endTime'
      GROUP BY sensor_id, UNIX_TIMESTAMP(date) DIV 900
      ORDER BY tms
      LIMIT 0, 5000;";
*/
try {
   $request = $db->query($sql);
   $rows = array();
   while($row = $request->fetch(PDO::FETCH_OBJ))
   {
      $rows[]="[".$row->tms.",".$row->avg."]"; 
//      $rows[]="[".$row->tms.",".$row->min.",".$row->max."]"; // pour courbe 'arearange'
   }

   echo "/* console.log(' start = $start, end = $end, startTime = $startTime, endTime = $endTime '); */\n"; 
   echo $callback ."([\n" . join(",\n", $rows) ."\n]);"; 
}
catch(PDOException $e)
{
   $error_msg=$e->getMessage();
   $db=null;
   die($error_msg);
}

$db=null;
?>
