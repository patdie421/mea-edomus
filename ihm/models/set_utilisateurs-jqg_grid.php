<?php
include_once('../lib/configs.php');

ob_start();
var_dump($_POST);
$contents = ob_get_contents();
ob_end_clean();
error_log($contents);

if(isset($_POST['oper'])){
    $oper = $_POST['oper'];
    $fields=array('oper','id_user','name','password','description','profil','flag');
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
$id_user = $_POST['id_user'];
$name = $_POST['name'];
$password = $_POST['password'];
$description = $_POST['description'];
$profil = $_POST['profil'];
$flag = $_POST['flag'];

try {
    $file_db = new PDO($PARAMS_DB_PATH);
}catch (PDOException $e){
    echo json_encode(array("error"=>2,"error_msg"=>$e->getMessage() ));
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT


if($oper === 'add'){
    $sql_insert="INSERT INTO users (id_user,name,password,description,profil,flag) "
               ."VALUES(:id_user, :name, :password, :description, :profil, :flag)";
    try{
        $stmt = $file_db->prepare($sql_insert);
        $stmt->execute(
            array(
                ":id_user"     => $id_user,
                ":name"        => $name,
                ":password"    => $password,
                ":description" => $description,
                ":profil"      => $profil,
                ":flag"        => $flag
            )
        );
     }catch(PDOException $e){
        error_log($e->getMessage());
        echo json_encode(array("error"=>3,"error_msg"=>$e->getMessage() ));
        $file_db=null;
        exit(1);
     }
}elseif($oper === 'edit'){
    $sql_update="UPDATE users SET "
                                 ."id_user=:id_user, "
                                 ."name=:name, "
                                 ."password=:password, "
                                 ."description=:description, "
                                 ."profil=:profil, "
                                 ."flag=:flag "
                                 
               ."WHERE id=:id";

    try{
        $stmt = $file_db->prepare($sql_update);
        $stmt->execute(
            array(
                ":id"          => $id,
                ":id_user"     => $id_user,
                ":name"        => $name,
                ":password"    => $password,
                ":description" => $description,
                ":profil"      => $profil,
                ":flag"        => $flag
            )
        );
    }catch(PDOException $e){
        error_log($e->getMessage());
        echo json_encode(array("error"=>4,"error_msg"=>$e->getMessage() ));
        $file_db=null;
        exit(1);
    }
}elseif($oper === 'del'){
    $sql_delete="DELETE FROM users WHERE id=:id";
    try{
        $stmt = $file_db->prepare($sql_delete);
        $stmt->execute(
            array(
                ":id" => $id
            )
        );
    }catch(PDOException $e){
        error_log($e->getMessage());
        echo json_encode(array("error"=>5,"error_msg"=>$e->getMessage() ));
        $file_db=null;
        exit(1);
    }
}

$file_db = null;