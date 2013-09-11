<?php
session_start();

header('Content-type: application/json');

if(isset($_GET['userid'])){
    $userid = trim($_GET['userid']);
}else{
    echo json_encode(array("retour"=>"KO","erreur"=>"userid absent"));
    exit();
}
if(isset($_GET['passwd'])){
    $passwd = $_GET['passwd'];
}else{
    echo json_encode(array("retour"=>"KO","erreur"=>"passwd absent"));
    exit();
}

if($userid == $passwd){
    $_SESSION['userid']=$userid;
    echo json_encode(array("retour"=>"OK"));
}else{
    session_destroy();
    echo json_encode(array("retour"=>"KO","erreur"=>"identifiant et/ou mot de passe incorrect"));
}
