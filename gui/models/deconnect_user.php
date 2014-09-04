<?php
session_start();
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');

switch(check_admin()){
    case 98:
        echo json_encode(array("result"=>"KO","error"=>98,"error_msg"=>"non habilité" ));
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

if(!isset($_GET['user'])){
    echo json_encode(array("result"=>"KO","error"=>2,"error_msg"=>"parameters error" ));
    exit(1);
}else{
    $user=$_GET['user'];
}

try{
    $file_db = new PDO($PARAMS_DB_PATH);
}catch(PDOException $e){
    echo json_encode(array("result"=>"KO","error"=>2,"error_msg"=>$e->getMessage() ));
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

$SQL="SELECT sessionid FROM sessions WHERE userid=\"$user\"";
error_log($SQL);
try{
    $stmt = $file_db->prepare($SQL);
    $stmt->execute();
    $result = $stmt->fetchAll();
}catch(PDOException $e){
    echo json_encode(array("result"=>"KO","error"=>3,"error_msg"=>$e->getMessage() ));
    $file_db=null;
    exit(1);
}

$my_session_id=session_id();
foreach ($result as $result_row){
    $sessionid=$result_row['sessionid'];
    if(isset($sessionid)){
        session_write_close();
        session_id($sessionid);
        session_start();
        del_cookie();
        session_destroy();
        del_from_sessions_table($file_db, $sessionid);
    }
}

session_id($my_session_id);
session_start();

echo json_encode(array("result"=>"OK","error"=>0,"error_msg"=>"" ));
$stmt = null;
$file_db = null;

exit(0);


function del_cookie() {
    if (ini_get("session.use_cookies")) {
        $params = session_get_cookie_params();
        setcookie(session_name(), '', time() - 42000,
            $params["path"], $params["domain"],
            $params["secure"], $params["httponly"]
        );
    }
}

function del_from_sessions_table($file_db,$sessionid)
{
    $SQL="DELETE FROM sessions WHERE sessionid=\"$sessionid\"";
    try{
        $stmt = $file_db->prepare($SQL);
        $stmt->execute();
    }catch(PDOException $e){
        echo json_encode(array("result"=>"KO","error"=>3,"error_msg"=>$e->getMessage() ));
        $file_db=null;
        exit(1);
    }
    $stmt=null;
}
