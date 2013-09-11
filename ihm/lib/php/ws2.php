<?php
//header('Content-type: application/json');

class vide
{
}

$page = $_GET['page']; // get the requested page
$limit = $_GET['rows']; // get how many rows we want to have into the grid
$sidx = $_GET['sidx']; // get index row - i.e. user click to sort
$sord = $_GET['sord']; // get the direction

if(!$sidx)
    $sidx =1;
    
$count = 40;

if( $count >0 ){
    $total_pages = ceil($count/$limit);
}
else{
    $total_pages = 0;
}

if ($page > $total_pages)
    $page=$total_pages;
    
$start = $limit*$page - $limit; // do not put $limit*($page - 1) 

if ( stristr($_SERVER["HTTP_ACCEPT"],"application/xhtml+xml") ){
    header("Content-type: application/xhtml+xml;charset=utf-8");
}else{
    header("Content-type: text/xml;charset=utf-8");
}
$et = ">";
echo "<?xml version='1.0' encoding='utf-8'?$et\n";
echo "<rows>";
echo "<page>".$page."</page>";
echo "<total>".$total_pages."</total>";
echo "<records>".$count."</records>"; // be sure to put text data in CDATA
for($i=$start;$i<($start+$limit) && $i<$count;$i++){
    echo "<row id='". $i."'>";
    echo "<cell>". $i."</cell>";
    echo "<cell>". $i."</cell>";
    echo "<cell><![CDATA[". $i."]]></cell>";
    echo "<cell>". $i."</cell>";
    echo "<cell>". $i."</cell>";
    echo "<cell>". $i."</cell>";
    echo "<cell><![CDATA[". $i."]]></cell>";
    echo "</row>";
}
echo "</rows>"; ?>
