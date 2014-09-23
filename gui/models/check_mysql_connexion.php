<?php
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');

session_start();

$check=check_admin();
error_log($check);
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

header('Content-type: application/json; charset=utf-8' );

if(!isset($_GET['server']) ||
   !isset($_GET['port']) ||
   !isset($_GET['base']) ||
   !isset($_GET['user']) ||
   !isset($_GET['password'])){
    echo json_encode(array("result"=>"KO", "error"=>1,"error_msg"=>"parameters error" ));
    exit(1);
}else{
    $server=$_GET['server'];
    $port=$_GET['port'];
    $base=$_GET['base'];
    $user=$_GET['user'];
    $password=$_GET['password'];
}

$dns = 'mysql:host='.$server.';dbname='.$base.";port=".$port;
try{
    $connection = new PDO(
        $dns,
        $user,
        $password, 
        array(
            PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
            PDO::ATTR_TIMEOUT => 3
        )
    );
}catch(PDOException $e){
    error_log(json_encode(array("result"=>"KO","error"=>2, "error_msg"=>utf8_encode( $e->getMessage() ))));
    echo json_encode(array("result"=>"KO","error"=>2, "error_msg"=>utf8_encode( $e->getMessage() )));
    exit(1);
}
echo json_encode(array("result"=>"OK"));

$connection=null;
?>
