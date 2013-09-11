<?php
include_once('../configs.php');

if(isset($_POST['oper'])){
    $oper = $_POST['oper'];
    $fields=array('oper','parameters','id');
}else{
    echo json_encode(array("error"=>1,"error_msg"=>"parameters error"));
    exit(1);
}

$fieldsNotSet=array();
foreach ($fields as $field){
    if(!isset($_POST[$field])){
        $fieldsNotSet[]=$field;
    }
}

if(count($fieldsNotSet)){
    echo json_encode(array("error"=>1,"error_msg"=>"parameters error","error_fields"=>$fieldsNotSet ));
    exit(1);
}

$id = $_POST['id'];
$parameters = $_POST['parameters'];

try {
    $file_db = new PDO($PARAMS_DB_PATH);
}catch (PDOException $e){
    echo json_encode(array("error"=>2,"error_msg"=>$e->getMessage() ));
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT


if($oper === 'edit'){
    $sql_update="UPDATE types SET parameters=:parameters "
               ."WHERE id=:id";

    try{
        $stmt = $file_db->prepare($sql_update);
        $stmt->execute(
            array(
                ":id"         => $id,
                ":parameters" => $parameters
            )
        );
    }catch(PDOException $e){
        error_log($e->getMessage());
        echo json_encode(array("error"=>4,"error_msg"=>$e->getMessage() ));
        $file_db=null;
        exit(1);
    }
}

$file_db = null;