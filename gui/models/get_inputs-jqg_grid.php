<?php
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');
session_start();

switch(check_admin()){
    case 98:
        break;
    case 99:
        echo json_encode(array("result"=>"KO","error"=>99,"error_msg"=>"non connecté" ));
        exit(1);
    case 0:
        break;
    default:
        echo json_encode(array("result"=>"KO","error"=>1,"error_msg"=>"erreur inconnue" ));
        exit(1);
}

$page = $_GET['page']; // get the requested page
$limit = $_GET['rows']; // get how many rows we want to have into the grid
$sidx = $_GET['sidx']; // get index row - i.e. user click to sort
$sord = $_GET['sord']; // get the direction

if(!$sidx)
    $sidx =1;

try{
    $file_db = new PDO($PARAMS_DB_PATH);
}catch(PDOException $e){
    echo json_encode(array("result"=>"KO","error"=>2,"error_msg"=>$e->getMessage() ));
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT
    
try{    
    $stmt = $file_db->prepare("SELECT count(*) AS count FROM (SELECT * FROM rules)");
    $stmt->execute();
    $result = $stmt->fetchAll();
}catch(PDOException $e){
    echo json_encode(array("result"=>"KO","error"=>3,"error_msg"=>$e->getMessage() ));
    $file_db=null;
    exit(1);
}
$count=$result[0]['count'];

if( $count >0 ) {
	$total_pages = ceil($count/$limit);
} else {
	$total_pages = 0;
}
if ($page > $total_pages) $page=$total_pages;
$start = $limit*$page - $limit; // do not put $limit*($page - 1)

//$SQL = "SELECT * FROM rules ORDER BY $sidx $sord LIMIT $start , $limit";
$SQL="SELECT rules.id as id,
             rules.name as name,
             rules.source as xplsource,
             rules.schema as xplschema,
             rules.nb_conditions as conditionslist,
             rules.input_index as inputindex
      FROM rules
      ORDER BY $sidx $sord LIMIT $start , $limit";

try{    
    $stmt = $file_db->prepare($SQL);
    $stmt->execute();
    $result = $stmt->fetchAll();
}catch(PDOException $e){
    echo json_encode(array("result"=>"KO","error"=>4,"error_msg"=>$e->getMessage() ));
    $file_db=null;
    exit(1);
}

$res="";
$et = ">";
$res.="<?xml version='1.0' encoding='utf-8'?$et\n";
$res.="<rows>";
$res.="<page>".$page."</page>";
$res.="<total>".$total_pages."</total>";
$res.="<records>".$count."</records>";

foreach ($result as $result_elem){
	$res.="<row id='".$result_elem['id']."'>";
   $res.="<cell>".$result_elem['id']."</cell>";
   $res.="<cell><![CDATA[".$result_elem['name']."]]></cell>";
   $res.="<cell><![CDATA[".$result_elem['xplsource']."]]></cell>";
   $res.="<cell><![CDATA[".$result_elem['xplschema']."]]></cell>";
   
   $id=$result_elem['id'];
   $SQL2="SELECT conditions.key as key,
      conditions.value as value,
      conditions.op as op
      FROM conditions
      WHERE id_rule=$id
      ORDER BY $sidx $sord LIMIT $start , $limit";

   try {
      $stmt2 = $file_db->prepare($SQL2);
      $stmt2->execute();
      $result2 = $stmt2->fetchAll();
   } catch(PDOException $e) {
      echo json_encode(array("result"=>"KO","error"=>4,"error_msg"=>$e->getMessage() ));
      $file_db=null;
      exit(1);
   }
   
   $conditions_list="";
   
   foreach ($result2 as $result2_elem) {
      $op="?";
      switch ($result2_elem['op']) {
         case 1:
            $op="=";
            break;
         case 2:
            $op="!=";
            break;
         case 3:
            $op="<";
            break;
         case 4:
            $op="<=";
            break;
         case 5:
            $op=">";
            break;
         case 6:
            $op=">=";
            break;
         default:
            $op="?";
            break;
      }
      $conditions_list.=$result2_elem['key']." ".$op." ";
      if (is_numeric($result2_elem['value'])) {
         $conditions_list.=$result2_elem['value'];
      }
      else {
         $conditions_list.="\"".$result2_elem['value']."\"";
      }
      $conditions_list.="; ";
   }

   $res.="<cell><![CDATA[".$conditions_list."]]></cell>";
   $res.="<cell><![CDATA[".$result_elem['inputindex']."]]></cell>";
   $res.="</row>";
}
$res.="</rows>";

if ( stristr($_SERVER["HTTP_ACCEPT"],"application/xhtml+xml") ) {
    header("Content-type: application/xhtml+xml;charset=utf-8");
}else{
    header("Content-type: text/xml;charset=utf-8");
}
echo $res;

$file_db = null;
