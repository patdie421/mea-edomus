<?php
session_start();
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');

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
    echo json_encode(array("error"=>1,"error_msg"=>$e->getMessage() ));
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT
    
try{    
    $stmt = $file_db->prepare("SELECT count(*) AS count FROM (SELECT * FROM users)");
    $stmt->execute();
    $result = $stmt->fetchAll();
}catch(PDOException $e){
    echo json_encode(array("error"=>2,"error_msg"=>$e->getMessage() ));
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

//$SQL = "SELECT * FROM interfaces ORDER BY $sidx $sord LIMIT $start , $limit";
$SQL="SELECT users.id as id,
             users.id_user as id_user,
             users.name as name,
             users.password as password,
             users.description as description,
             users.profil as profil,
             users.flag as flag
      FROM users
      ORDER BY $sidx $sord LIMIT $start , $limit";

try{    
    $stmt = $file_db->prepare($SQL);
    $stmt->execute();
    $result = $stmt->fetchAll();
}catch(PDOException $e){
    echo json_encode(array("error"=>3,"error_msg"=>$e->getMessage() ));
    $file_db=null;
    exit(1);
}

if ( stristr($_SERVER["HTTP_ACCEPT"],"application/xhtml+xml") ) {
    header("Content-type: application/xhtml+xml;charset=utf-8");
}else{
    header("Content-type: text/xml;charset=utf-8");
}
$et = ">";

echo "<?xml version='1.0' encoding='utf-8'?$et\n";
echo "<rows>";
echo "<page>".$page."</page>";
echo "<total>".$total_pages."</total>";
echo "<records>".$count."</records>";

foreach ($result as $result_elem){
	echo "<row id='".$result_elem['id']."'>";
    echo "<cell>".$result_elem['id']."</cell>";
    echo "<cell>".$result_elem['id_user']."</cell>";
    echo "<cell><![CDATA[".$result_elem['name']."]]></cell>";
    echo "<cell><![CDATA[".$result_elem['password']."]]></cell>";
    echo "<cell><![CDATA[".$result_elem['description']."]]></cell>";
    echo "<cell>".$result_elem['profil']."</cell>";
    echo "<cell>".$result_elem['flag']."</cell>";
    echo "</row>";
}
echo "</rows>";

$file_db = null;