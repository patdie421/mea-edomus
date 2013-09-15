<?php
include_once('../lib/configs.php');

$page = $_GET['page']; // get the requested page
$limit = $_GET['rows']; // get how many rows we want to have into the grid
$sidx = $_GET['sidx']; // get index row - i.e. user click to sort
$sord = $_GET['sord']; // get the direction

error_log($sidx);
error_log($sord);

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
    $stmt = $file_db->prepare("SELECT count(*) AS count FROM (SELECT * FROM types)");
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

$SQL="SELECT types.id as id,
             types.id_type as id_type,
             types.name as name,
             types.description as description,
             types.parameters as parameters,
             types.flag as flags
      FROM types
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

//ob_start();
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
    if($result_elem['flags']==1)
        $type_of_type="Standards";
    else
        $type_of_type="Personnalisés";
    
	echo "<row id='".$result_elem['id']."'>";
    echo "<cell>".$result_elem['id']."</cell>";
    echo "<cell>".$result_elem['id_type']."</cell>";
    echo "<cell><![CDATA[".$result_elem['name']."]]></cell>";
    echo "<cell><![CDATA[".$result_elem['description']."]]></cell>";
    echo "<cell><![CDATA[".$result_elem['parameters']."]]></cell>";
    echo "<cell><![CDATA[".$type_of_type."]]></cell>";
    echo "</row>";
}
echo "</rows>";
/*
$contents = ob_get_contents();
ob_end_clean();
error_log($contents);
*/
$file_db = null;