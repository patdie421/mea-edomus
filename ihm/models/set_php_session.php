<?php
session_start();

header('Content-type: application/json');

$var = $_GET['_var'];
$val = $_GET['_val'];

$_SESSION[$var]=$val;

echo json_encode(array("retour"=>"OK"));
