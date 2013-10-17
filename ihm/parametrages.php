<?php
//
//  PAGE PRINCIPALE (VIEW) : paramétrage de l'applications : capteurs/actionneurs, interfaces, types, lieux
//
session_start();
?>
<!DOCTYPE html>
<?php
include "lib/configs.php";

    if(!isset($_SESSION['logged_in']))
    {
        $dest=$_SERVER['PHP_SELF'];
        echo "<script>window.location = \"login.php?dest=$dest\";</script>";
        exit();
    }
    
    if(!isset($_SESSION['profil']) || $_SESSION['profil']!=1){
        echo "<script>window.location = \"index.php\";</script>";
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
            <div id="objects_admin" style="padding-left:170px; text-align:center; font-size:18px;">
            Objects administration
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
        PIEDPAGE
        </div>
    </div>
</body>
    
<script type="text/javascript" src="lib/js/strings.js"></script>
<script type="text/javascript">
jQuery(document).ready(function(){
    $.ajaxSetup({ cache: false });
    
    // traduction
    $("#objects_admin").text(str_objects_admin.capitalize());
    
    // chargement des sous-pages
    $("#entete").load("views/commun/page-entete.php");
    $("#menu").load("views/commun/page-menu.php");
    $("#contenu").load("views/parametrages-contenu.php");
    $("#piedpage").load("views/commun/page-pied.php");
});
</script>
</html>
