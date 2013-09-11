<?php
include_once('../configs.php');

ob_start();
var_dump($_POST);
$contents = ob_get_contents();
ob_end_clean();
error_log($contents);

if(isset($_POST['oper'])){
    $oper = $_POST['oper'];
    $fields=array('oper','id_location','name','description');
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
$id_location = $_POST['id_location'];
$name = strtoupper($_POST['name']);
$description = $_POST['description'];

try {
    $file_db = new PDO($PARAMS_DB_PATH);
}catch (PDOException $e){
    echo json_encode(array("error"=>2,"error_msg"=>$e->getMessage() ));
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT


if($oper === 'add'){
    $sql_insert="INSERT INTO locations (id_location,name,description) "
               ."VALUES(:id_location, :name, :description)";
    try{
        $stmt = $file_db->prepare($sql_insert);
        $stmt->execute(
            array(
                ":id_location" => $id_location,
                ":name"        => $name,
                ":description" => $description,
            )
        );
     }catch(PDOException $e){
        echo json_encode(array("error"=>3,"error_msg"=>$e->getMessage() ));
        $file_db=null;
        exit(1);
     }
}elseif($oper === 'edit'){
    $sql_update="UPDATE locations SET "
                                 ."id_location=:id_location, "
                                 ."name=:name, "
                                 ."description=:description "
               ."WHERE id=:id";

    try{
        $stmt = $file_db->prepare($sql_update);
        $stmt->execute(
            array(
                ":id"          => $id,
                ":id_location" => $id_location,
                ":name"        => $name,
                ":description" => $description
            )
        );
    }catch(PDOException $e){
        error_log($e->getMessage());
        echo json_encode(array("error"=>4,"error_msg"=>$e->getMessage() ));
        $file_db=null;
        exit(1);
    }
}elseif($oper === 'del'){
    $sql_delete="DELETE FROM locations WHERE id=:id";
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