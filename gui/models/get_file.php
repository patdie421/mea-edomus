<?php
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');
session_start();

if(isset($DEBUG_ON) && ($DEBUG_ON == 1))
{
   ob_start();
   print_r($_REQUEST);
   $debug_msg = ob_get_contents();
   ob_end_clean();
   error_log($debug_msg);
}

header('Content-type: application/json');

switch(check_admin()){
    case 98:
        break;
    case 99:
        echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>99, "errMsg"=>"not connected" ));
        exit(1);
    case 0:
        break;
    default:
        echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>1, "errMsg"=>"unknown error" ));
        exit(1);
}

if(!isset($_GET['type']) || !isset($_GET['name'])){
    echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>2, "errMsg"=>"parameters error" ));
    exit(1);
}else{
    $type=$_GET['type'];
    $name=$_GET['name'];
}


function endsWith($haystack, $needle) {
    // search forward starting from end minus needle length characters
    return $needle === "" || (($temp = strlen($haystack) - strlen($needle)) >= 0 && strpos($haystack, $needle, $temp) !== FALSE);
}


function getPath($db, $param)
{
   $response=[];

   try {
      $file_db = new PDO($db);
   }
   catch (PDOException $e) {
      return false;
   }

   $file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
   $file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

   $SQL='SELECT * FROM application_parameters WHERE key = "'.$param.'"';
   try {
      $stmt = $file_db->prepare($SQL);
      $stmt->execute();
      $result = $stmt->fetchAll();
   }
   catch(PDOException $e) {
      $result = false;
   }
   $file_db=null;
   return $result;
}

$param=false;
$path=false;
if($type=='srules')
{
   $param="RULESFILESPATH";
   $extention="srules";
}
else
{
   echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>3, "errMsg"=>"type unknown" ));
   exit(1);
}

if($param!=false)
{
   $res=getPath($PARAMS_DB_PATH, $param);
   if($res!=false)
   {
      $res=$res[0];
      $path=$res{'value'};
   }
}
else
{
   echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>4, "errMsg"=>"can't get parameter" ));
   exit(1);
}

set_error_handler(
    create_function(
        '$severity, $message, $file, $line',
        'throw new ErrorException($message, $severity, $severity, $file, $line);'
    )
);
try {
   $file=file_get_contents($path . "/" . $name);
}
catch(Exception $e) {
   echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>5, "errMsg"=>"can't get file - " . $e->getMessage() ));
   exit(1);
}
restore_error_handler();

echo json_encode(array('iserror'=>false, "result"=>"OK","file"=>$file, "errno"=>0));
