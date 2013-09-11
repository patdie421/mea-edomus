<?php
session_start();
?>

<!DOCTYPE html>

<?php
    if(!isset($_SESSION['userid']))
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
        .ui-widget{font-size:12px;}
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
    
    <script type="text/javascript">
        $.ajaxSetup({ cache: false });

        $(function(){
            $('#entete').load('sub-pages/commun/page-entete.php');
            $("#menu").load('sub-pages/commun/page-menu.php');
            $("#contenu").load('sub-pages/commandes-contenu.php');
            $("#piedpage").load('sub-pages/commun/page-pied.php');
        });
    </script>
</body>
</html>