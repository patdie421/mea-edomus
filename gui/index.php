<?php
include "lib/configs.php";
//
//  PAGE PRINCIPALE (VIEW) : home page
//
session_start();
?>
<!DOCTYPE html>
<?php

// contrôle et redirections
if(!isset($_SESSION['logged_in']))
{
    $dest=$_SERVER['PHP_SELF'];
    echo "<script>window.location = \"login.php?dest=$dest\";</script>";
    exit();
}
?>

<html>
<head>
    <meta charset="utf-8">
    <title>
    <?php echo $TITRE_APPLICATION; ?>
    </title>
    <?php include "lib/includes.php"; ?>
</head>
<body>
    <style>
        .ui-widget{font-size:14px;}
    </style>
    
    <div>
        <div id='entete'>
        ENTETE
        </div>
        <div id='titre'>
            <div style="padding-left:170px; text-align:center;">
            TITRE
            </div>
        </div>
        <div id='main'>
            <div id='menu'>
            MENU
            </div>
            <div id='contenu'>
            CONTENU
            </div>
        </div>
        <div id='piedpage'>
        PIED
        </div>
    </div>
</body>
<script type="text/javascript" src="lib/js/mea-gui-utils.js"></script>
<script type="text/javascript" src="lib/js/strings.js"></script>
<script type="text/javascript">
jQuery(document).ready(function(){
    $.ajaxSetup({ cache: false });
    $("#entete").load("views/commun/page-entete.php");
    $("#menu").load("views/commun/page-menu.php");
    $("#contenu").load("views/index-contenu.php");
    $("#piedpage").load("views/commun/page-pied.php");
});
</script>
</html>
