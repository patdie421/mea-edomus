<?php
//
//  PAGE PRINCIPALE (VIEW) : configuration de l'application
//
session_start();
?>
<!DOCTYPE html>
<?php
include "lib/configs.php";

// contrôles (login et habilitation) et redirections
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
    <div>
        <div id='entete'>
        ENTETE
        </div>
        <div id='titre'>
            <div style="padding-left:170px; text-align:center;" id="application_prefs">
            Application preferences
            </div>
        </div>
        <div id='main'>
            <div id='menu'>
            MENU
            </div>
            <div id='contenu'>
            CONTENU
        </div>
        <div id='piedpage'>
        PIED
        </div>
    </div>
</body>
<script type="text/javascript" src="lib/js/strings.js"></script>
<script type="text/javascript">
jQuery(document).ready(function(){
    $.ajaxSetup({ cache: false });
    
//
// initialisation de l'interface
//
    // chargement des sous-pages
    $("#application_prefs").text(str_application_prefs.capitalize());
    $("#entete").load("views/commun/page-entete.php");
    $("#menu").load("views/commun/page-menu.php");
    $("#contenu").load("views/config-contenu.php");
    $("#piedpage").load("views/commun/page-pied.php");
});
</script>
</html>