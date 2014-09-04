<?php
session_start();
include_once('../lib/configs.php');

if( !isset($_SESSION['userid']) ||
    !isset($_SESSION['logged_in']) ||
    !isset($_SESSION['profil'])) {
        echo json_encode(array("result"=>"KO","error"=>99,"error_msg"=>"non connecté" ));
        exit(1);
}

echo json_encode(array("result"=>"OK", "userid"=>$_SESSION['userid'], "logged_in"=>$_SESSION['logged_in'], "profil"=>$_SESSION['profil']));

$file_db = null;