<?php
session_start();
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');

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

if(!isset($_GET['table']) || !isset($_GET['field']) || !isset($_GET['where'])){
    echo json_encode(array("result"=>"KO","error"=>2,"error_msg"=>"parameters error" ));
    exit(1);
}else{
    $table=$_GET['table'];
    $field=$_GET['field'];
    $where=$_GET['where'];
}

$id=0;
if(isset($_GET['id'])){
    $id=$_GET['id'];
}

try {
    $file_db = new PDO($PARAMS_DB_PATH);
}catch (PDOException $e){
    echo json_encode(array("result"=>"KO","error"=>3,"error_msg"=>$e->getMessage() ));
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

$SQL="SELECT $field FROM $table WHERE $where";
try{    
    $stmt = $file_db->prepare($SQL);
    $stmt->execute();
    $result = $stmt->fetchAll();
}catch(PDOException $e){
    echo json_encode(array("result"=>"KO","error"=>4,"error_msg"=>$e->getMessage() ));
    $file_db=null;
    exit(1);
}

$values=array();
foreach ($result as $elem){
    $values[]=$elem[$field];
}

header('Content-type: application/json');

echo json_encode(array("result"=>"OK","values"=>$values,"error"=>0));

$file_db = null;