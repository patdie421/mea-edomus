<?php
include_once('../configs.php');

if(!isset($_GET['table']) || !isset($_GET['id'])){
    echo json_encode(array("error"=>1,"error_msg"=>"parameters error" ));
    exit(1);
}else{
    $table=$_GET['table'];
    $id=$_GET['id'];
}

try {
    $file_db = new PDO($PARAMS_DB_PATH);
}catch (PDOException $e){
    echo json_encode(array("error"=>2,"error_msg"=>$e->getMessage() ));
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

$SQL="DELETE FROM $table WHERE id=$id";
try{    
    $stmt = $file_db->prepare($SQL);
    $stmt->execute();
    $result = $stmt->fetchAll();
}catch(PDOException $e){
    echo json_encode(array("error"=>3,"error_msg"=>$e->getMessage() ));
    $file_db=null;
    exit(1);
}

header('Content-type: application/json');

echo json_encode(array("error"=>0));

$file_db = null;