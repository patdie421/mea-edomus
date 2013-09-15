<?php
session_start();

header('Content-type: application/json');

session_destroy();

echo json_encode(array("retour"=>"OK"));
