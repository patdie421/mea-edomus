<?php
include_once('../configs.php');


if(isset($_POST['oper'])){
    $oper = $_POST['oper'];
    $fields=array('oper','id_sensor_actuator','id_interface','id_type','name','description','parameters','state','id_location');
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
$id_sensor_actuator = $_POST['id_sensor_actuator'];
$id_interface = $_POST['id_interface'];
$id_type = $_POST['id_type'];
$name = strtoupper($_POST['name']);
$description = $_POST['description'];
$parameters = $_POST['parameters'];
$state = $_POST['state'];
$id_location = $_POST['id_location'];

try {
    $file_db = new PDO($PARAMS_DB_PATH);
}catch (PDOException $e){
    echo json_encode(array("error"=>2,"error_msg"=>$e->getMessage() ));
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT


if($oper === 'add'){
    $sql_insert="INSERT INTO sensors_actuators (id_sensor_actuator,id_interface,id_type,name,description,parameters,state,id_location) "
               ."VALUES(:id_sensor_actuator, :id_interface, :id_type, :name, :description, :parameters, :state, :id_location)";
    try{
        $stmt = $file_db->prepare($sql_insert);
        $stmt->execute(
            array(
                ":id_sensor_actuator" => $id_sensor_actuator,
                ":id_interface"       => $id_interface,
                ":id_type"            => $id_type,
                ":name"               => $name,
                ":description"        => $description,
                ":parameters"         => $parameters,
                ":state"              => $state,
                ":id_location"        => $id_location
            )
        );
     }catch(PDOException $e){
        echo json_encode(array("error"=>3,"error_msg"=>$e->getMessage() ));
        $file_db=null;
        exit(1);
     }
}elseif($oper === 'edit'){
    $sql_update="UPDATE sensors_actuators SET "
                                 ."id_sensor_actuator=:id_sensor_actuator, "
                                 ."id_interface=:id_interface, "
                                 ."id_type=:id_type, "
                                 ."name=:name, "
                                 ."description=:description, "
                                 ."parameters=:parameters, "
                                 ."state=:state, "
                                 ."id_location=:id_location "
               ."WHERE id=:id";
    try{
        $stmt = $file_db->prepare($sql_update);
        $stmt->execute(
            array(
                ":id"                 => $id,
                ":id_sensor_actuator" => $id_sensor_actuator,
                ":id_interface"       => $id_interface,
                ":id_type"            => $id_type,
                ":name"               => $name,
                ":description"        => $description,
                ":parameters"         => $parameters,
                ":state"              => $state,
                ":id_location"        => $id_location
            )
        );
    }catch(PDOException $e){
        error_log($e->getMessage());
        echo json_encode(array("error"=>4,"error_msg"=>$e->getMessage() ));
        $file_db=null;
        exit(1);
    }
}elseif($oper === 'del'){
    $sql_delete="DELETE FROM sensors_actuators WHERE id=:id";
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