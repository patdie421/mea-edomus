<?php
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');
session_start();

$check=check_admin();
switch($check){
    case 98:
        echo json_encode(array("result"=>"KO","error"=>98,"error_msg"=>"pas habilité" ));
        exit(1);
    case 99:
        echo json_encode(array("result"=>"KO","error"=>99,"error_msg"=>"non connecté" ));
        exit(1);
    case 0:
        break;
    default:
        echo json_encode(array("result"=>"KO","error"=>1,"error_msg"=>"erreur inconnue (".$check.")." ));
        exit(1);
}
/*
ob_start();
var_dump($_POST);
$contents = ob_get_contents();
ob_end_clean();
error_log($contents);
*/
if(isset($_POST['oper'])){
    $oper = $_POST['oper'];
    $fields=array('oper','id_user','name','password','description','profil','flag');
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
$id_user = $_POST['id_user'];
$name = $_POST['name'];
$password = $_POST['password'];
$description = $_POST['description'];
$profil = $_POST['profil'];
$flag = $_POST['flag'];

try {
    $file_db = new PDO($PARAMS_DB_PATH);
}catch (PDOException $e){
    echo json_encode(array("result"=>"KO","error"=>4,"error_msg"=>$e->getMessage() ));
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
        echo json_encode(array("result"=>"KO","error"=>5,"error_msg"=>$e->getMessage() ));
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
        echo json_encode(array("result"=>"KO","error"=>6,"error_msg"=>$e->getMessage() ));
        $file_db=null;
        exit(1);
    }

    $SQL="SELECT sessionid FROM sessions WHERE userid=\"$name\"";
    try{
        $stmt2 = $file_db->prepare($SQL);
        $stmt2->execute();
        $result = $stmt2->fetchAll();
    }catch(PDOException $e){
        error_log($e->getMessage());
        echo json_encode(array("result"=>"KO","error"=>3,"error_msg"=>$e->getMessage() ));
        $file_db=null;
        exit(1);
    }
    $stmt2=null;

    $my_session_id=session_id();
    foreach ($result as $result_row){
        $sessionid=$result_row['sessionid'];
        if(isset($sessionid)){
            session_write_close();
            session_id($sessionid);
            session_start();
            error_log($_SESSION['userid']." ".$_SESSION['profil']." ".$profil);
            $_SESSION['profil']=$profil;
            $_SESSION['flag']=$flag;
        }
    }
    session_write_close();
    session_id($my_session_id);
    session_start();

    // boucle, voir deconnect_user.php
    
}elseif($oper === 'del'){
    $sql_delete="DELETE FROM users WHERE id=:id";
    error_log($sql_delete);
    try{
        $stmt = $file_db->prepare($sql_delete);
        $stmt->execute(
            array(
                ":id" => $id
            )
        );
    }catch(PDOException $e){
        error_log($e->getMessage());
        echo json_encode(array("result"=>"KO","error"=>7,"error_msg"=>$e->getMessage() ));
        $file_db=null;
        exit(1);
    }
}

$file_db = null;
