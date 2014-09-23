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

try{
    $stmt = $file_db->prepare("SELECT id_interface,name FROM interfaces");
    $stmt->execute();
    $result = $stmt->fetchAll();
}catch(PDOException $e){
    echo json_encode(array("result"=>"KO","error"=>3,"error_msg"=>$e->getMessage() ));
    $file_db=null;
    exit(1);
}

$reponse="<select>";
foreach ($result as $result_row){
    $reponse .= '<option value='.$result_row['id_interface'].'>'.$result_row['name'].'</option>';
}
$reponse .= '</select>';

 echo $reponse;

 $file_db = null;
?>
