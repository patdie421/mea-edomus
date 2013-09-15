<?php
include_once('../lib/configs.php');

try {
    $file_db = new PDO($PARAMS_DB_PATH);
}catch (PDOException $e){
    echo json_encode(array("error"=>1,"error_msg"=>$e->getMessage() ));
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

try{      
    $stmt = $file_db->prepare("SELECT MAX(id_location) AS max_id FROM locations");
    $stmt->execute();
    $result = $stmt->fetchAll();
}catch(PDOException $e){
    echo json_encode(array("error"=>2,"error_msg"=>$e->getMessage() ));
    $file_db=null;
    exit(1);
}
$next_id=$result[0]['max_id']+1;

header('Content-type: application/json');

echo json_encode(array("next_id"=>$next_id));

$file_db = null;
