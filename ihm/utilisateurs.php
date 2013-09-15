<?php
session_start();
?>

<!DOCTYPE html>

<?php
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
    <title>MES TEST JQUERY</title>
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
            <div style="padding-left:170px; text-align:center; font-size:18px;">
            Administration des utilisateurs
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
    
    <script>
        $.ajaxSetup({ cache: false });

        $(function(){
            $("#entete").load("views/commun/page-entete.php");
            $("#menu").load("views/commun/page-menu.php");
            $("#contenu").load("views/utilisateurs-contenu.php");
            $("#piedpage").load("views/commun/page-pied.php");
        });
    </script>
</body>
</html>
