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

include_once('../lib/configs.php');

if(!isset($_GET['table1']) ||
   !isset($_GET['field1']) ||
   !isset($_GET['where1']) ||
   !isset($_GET['table2']) ||
   !isset($_GET['field2']) ||
   !isset($_GET['where2'])){
    echo json_encode(array("result"=>"KO","error"=>2,"error_msg"=>"parameters error" ));
    exit(1);
}else{
    $table1=$_GET['table1'];
    $field1=$_GET['field1'];
    $where1=$_GET['where1'];
    $table2=$_GET['table2'];
    $field2=$_GET['field2'];
    $where2=$_GET['where2'];
}

try {
    $file_db = new PDO($PARAMS_DB_PATH);
}catch (PDOException $e){
    echo json_encode(array("result"=>"KO","error"=>3,"error_msg"=>$e->getMessage() ));
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

$SQL="SELECT $field1 FROM $table1 WHERE $where1";
try{    
    $stmt = $file_db->prepare($SQL);
    $stmt->execute();
    $result1 = $stmt->fetchAll();
}catch(PDOException $e){
    echo json_encode(array("result"=>"KO","error"=>4,"error_msg"=>$e->getMessage() ));
    $file_db=null;
    exit(1);
}

$SQL="SELECT $field2 FROM $table2 WHERE $where2";
try{    
    $stmt = $file_db->prepare($SQL);
    $stmt->execute();
    $result2 = $stmt->fetchAll();
}catch(PDOException $e){
    echo json_encode(array("result"=>"KO","error"=>5,"error_msg"=>$e->getMessage() ));
    $file_db=null;
    exit(1);
}

$values1=array();
foreach ($result1 as $elem){
    $values1[]=$elem[$field1];
}

$values2=array();
foreach ($result2 as $elem){
    $values2[]=$elem[$field2];
}

header('Content-type: application/json');

echo json_encode(array("values1"=>$values1, "values2"=>$values2, "error"=>0));

$file_db = null;
