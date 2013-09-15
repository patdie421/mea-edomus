<?php
include_once('../lib/configs.php');

if(isset($_POST['oper'])){
    $oper = $_POST['oper'];
    $fields=array('oper','id_type','name','description','parameters');
    if($oper==='edit')
        $fields[]='id';
    if($oper==='del')
        $fields=array('oper','id');
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

if(isset($_POST['id']))
    $id = $_POST['id'];
$id_type = $_POST['id_type'];
$name = strtoupper($_POST['name']);
$description = $_POST['description'];
$parameters = $_POST['parameters'];

try {
    $file_db = new PDO($PARAMS_DB_PATH);
}catch (PDOException $e){
    echo json_encode(array("error"=>2,"error_msg"=>$e->getMessage() ));
    error_log($e->getMessage());
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

if($oper === 'add'){
    $sql_insert="INSERT INTO types (id_type,name,description,parameters) "
               ."VALUES(:id_type, :name, :description, :parameters)";
               
    try{
        $stmt = $file_db->prepare($sql_insert);
        $stmt->execute(
            array(
                ":id_type"     => $id_type,
                ":name"        => $name,
                ":description" => $description,
                ":parameters"  => $parameters,
            )
        );
     }catch(PDOException $e){
        echo json_encode(array("error"=>3,"error_msg"=>$e->getMessage() ));
        $file_db=null;
        exit(1);
     }
}elseif($oper === 'edit'){
    $sql_update="UPDATE types SET "
                                 ."id_type=:id_type, "
                                 ."name=:name, "
                                 ."description=:description, "
                                 ."parameters=:parameters "
               ."WHERE id=:id";

    try{
        $stmt = $file_db->prepare($sql_update);
        $stmt->execute(
            array(
                ":id"          => $id,
                ":id_type"     => $id_type,
                ":name"        => $name,
                ":description" => $description,
                ":parameters"  => $parameters
            )
        );
    }catch(PDOException $e){
        error_log($e->getMessage());
        echo json_encode(array("error"=>4,"error_msg"=>$e->getMessage() ));
        $file_db=null;
        exit(1);
    }
}elseif($oper === 'del'){
    $sql_delete="DELETE FROM types WHERE id=:id";
    try{
        $stmt = $file_db->prepare($sql_delete);
        $stmt->execute(
            array(
                ":id" => $id
            )
        );
    }catch(PDOException $e){
        echo json_encode(array("error"=>5,"error_msg"=>$e->getMessage() ));
        $file_db=null;
        exit(1);
    }
}

$file_db = null;