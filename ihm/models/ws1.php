<?php
header('Content-type: application/json');

echo json_encode(array("retour"=>"OK","device"=>$_GET['device']));
