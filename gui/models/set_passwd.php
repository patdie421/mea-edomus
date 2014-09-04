<?php
session_start();

include_once('../lib/configs.php');

if($_SESSION['flag']==2) {
    echo json_encode(array("error"=>1,"error_msg"=>"changement interdit" ));
}

if(isset($_GET['new_password'])){
    $new_password=$_GET['new_password'];
}
if(isset($_GET['old_password'])){
    $old_password=$_GET['old_password'];
}

$user_name=$_SESSION['userid'];

if(isset($_SESSION['logged_in']))
{
    $SQL_UPDATE="UPDATE users SET password=\"$new_password\" WHERE name=\"$user_name\" AND password=\"$old_password\"";
}else{
    $SQL_UPDATE="UPDATE users SET password=\"$new_password\", flag=0 WHERE name=\"$user_name\"";
}

header('Content-type: application/json');

try {
    $file_db = new PDO($PARAMS_DB_PATH);
}catch (PDOException $e){
    echo json_encode(array("error"=>2,"error_msg"=>$e->getMessage() ));
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

try{
    $stmt = $file_db->prepare($SQL_UPDATE);
    $stmt->execute();
}catch(PDOException $e){
    echo json_encode(array("error"=>3,"error_msg"=>$e->getMessage() ));
    $file_db=null;
    exit(1);
}

if($stmt->rowCount()>0){
    echo json_encode(array("retour"=>1));
    $_SESSION['logged_in']=1;
}else{
    echo json_encode(array("retour"=>0));
}

$file_db = null;
