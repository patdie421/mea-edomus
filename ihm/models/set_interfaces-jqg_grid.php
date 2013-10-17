<?php
session_start();
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');

switch(check_admin()){
    case 98:
        echo json_encode(array("result"=>"KO","error"=>98,"error_msg"=>"pas habilité" ));
        exit(1);
    case 99:
        echo json_encode(array("result"=>"KO","error"=>99,"error_msg"=>"non connecté" ));
        exit(1);
    case 0:
        break;
    default:
        echo json_encode(array("result"=>"KO","error"=>1,"error_msg"=>"erreur inconnue" ));
        exit(1);
}

if(isset($_POST['oper'])){
    $oper = $_POST['oper'];
    $fields=array('oper','id_interface','id_type','name','description','dev','parameters','state');
    if($oper==='edit')
        $fields[]='id';
    if($oper==='del')
        $fields=array('oper','id');
}else{
    echo json_encode(array("result"=>"KO","error"=>2,"error_msg"=>"parameters error"));
    exit(1);
}

$fieldsNotSet=array();
foreach ($fields as $field){
    if(!isset($_POST[$field])){
        $fieldsNotSet[]=$field;
    }
}

if(count($fieldsNotSet)){
    echo json_encode(array("result"=>"KO","error"=>3,"error_msg"=>"parameters error","error_fields"=>$fieldsNotSet ));
    exit(1);
}

if(isset($_POST['id']))
    $id = $_POST['id'];
$id_interface = $_POST['id_interface'];
$id_type = $_POST['id_type'];
$name = strtoupper($_POST['name']);
$description = $_POST['description'];
$dev = $_POST['dev'];
$parameters = $_POST['parameters'];
$state = $_POST['state'];

try {
    $file_db = new PDO($PARAMS_DB_PATH);
}catch (PDOException $e){
    echo json_encode(array("result"=>"KO","error"=>4,"error_msg"=>$e->getMessage() ));
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT


if($oper === 'add'){
    $sql_insert="INSERT INTO interfaces (id_interface,id_type,name,description,dev,parameters,state) "
               ."VALUES(:id_interface, :id_type, :name, :description, :dev, :parameters, :state)";
    try{
        $stmt = $file_db->prepare($sql_insert);
        $stmt->execute(
            array(
                ":id_interface" => $id_interface,
                ":id_type"      => $id_type,
                ":name"         => $name,
                ":description"  => $description,
                ":dev"          => $dev,
                ":parameters"   => $parameters,
                ":state"        => $state
            )
        );
     }catch(PDOException $e){
        echo json_encode(array("result"=>"KO","error"=>3,"error_msg"=>$e->getMessage() ));
        $file_db=null;
        exit(1);
     }
}elseif($oper === 'edit'){
    $sql_update="UPDATE interfaces SET "
                                 ."id_interface=:id_interface, "
                                 ."id_type=:id_type, "
                                 ."name=:name, "
                                 ."description=:description, "
                                 ."dev=:dev, "
                                 ."parameters=:parameters, "
                                 ."state=:state "
               ."WHERE id=:id";
    try{
        $stmt = $file_db->prepare($sql_update);
        $stmt->execute(
            array(
                ":id"           => $id,
                ":id_interface" => $id_interface,
                ":id_type"      => $id_type,
                ":name"         => $name,
                ":description"  => $description,
                ":dev"          => $dev,
                ":parameters"   => $parameters,
                ":state"        => $state
            )
        );
    }catch(PDOException $e){
        echo json_encode(array("result"=>"KO","error"=>5,"error_msg"=>$e->getMessage() ));
        $file_db=null;
        exit(1);
    }
}elseif($oper === 'del'){
    $sql_delete="DELETE FROM interfaces WHERE id=:id";
    try{
        $stmt = $file_db->prepare($sql_delete);
        $stmt->execute(
            array(
                ":id" => $id
            )
        );
    }catch(PDOException $e){
        echo json_encode(array("result"=>"KO","error"=>6,"error_msg"=>$e->getMessage() ));
        $file_db=null;
        exit(1);
    }
}

$file_db = null;