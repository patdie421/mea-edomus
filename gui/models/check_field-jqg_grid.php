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

if(!isset($_GET['table']) || !isset($_GET['field']) || !isset($_GET['value'])){
    echo json_encode(array("result"=>"KO","error"=>2,"error_msg"=>"parameters error" ));
    exit(1);
}else{
    $table=$_GET['table'];
    $index=$_GET['field'];
    $value=$_GET['value'];
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

try {
    if($id){
        $SQL="SELECT count(*) AS exist FROM $table WHERE $index=\"$value\" AND id<>$id";
    }
    else{
        $SQL="SELECT count(*) AS exist FROM $table WHERE $index=\"$value\"";
    }
    error_log($SQL);
    $stmt = $file_db->prepare($SQL);
    $stmt->execute();
    $result = $stmt->fetchAll();
}catch(PDOException $e){
    echo json_encode(array("result"=>"KO","error"=>4,"error_msg"=>$e->getMessage(),"debug"=>$SQL ));
    $file_db=null;
    exit(1);
}
error_log($result[0]['exist']);
$exist=$result[0]['exist'];

header('Content-type: application/json');

echo json_encode(array("exist"=>$exist,"error"=>0));

$file_db = null;
