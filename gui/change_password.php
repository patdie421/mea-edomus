<?php
include "lib/configs.php";
//
//  PAGE PRINCIPALE (VIEW) : changement du mot de passe de l'utilisateur connecté
//
session_start();
?>
<!DOCTYPE html>
<?php

// contrôle et redirections
if(isset($_SESSION['change_passwd_flag'])){
    if($_SESSION['change_passwd_flag']==0){
        session_destroy();
        echo "<script>";
        echo "window.location = \"login.php";
        if(isset($_GET['dest'])){
            $dest=$_GET['dest'];
            echo "?dest=\"$dest";
        }
        echo "\";";
        echo "</script>";
        exit(1);
    }
}elseif(!isset($_SESSION['logged_in'])){
    echo "<script>";
    echo "window.location = \"login.php\";";
    echo "</script>";
    exit(1);
}

if(isset($_SESSION['change_passwd_flag']))
    $_SESSION['change_passwd_flag']=0;
?>


<html>
<head>
    <title>
    <?php echo $TITRE_APPLICATION; ?>
    </title>
    <meta charset="utf-8">
    <style>
.form-chgpasswd {
    position: relative;
    margin: 0 auto;
    margin-top:50px;
    padding: 20px 20px 20px;
    width: 310px;

    border-color:#a6c9e2;
    border-width:1px;
    border-style:solid;
    }
    
.form-chgpasswd p.submit {
    text-align:center;}

.form-chgpasswd h1 {
    font-family: 'Lucida Grande', Tahoma, Verdana, sans-serif;
    font-size: 18px;
    margin: -20px -20px 21px;
    line-height: 40px;
    font-size: 15px;
    font-weight: bold;
    text-align: center;
    }

.form-chgpasswd input[type=text], .form-chgpasswd input[type=password] {
    font-family: 'Lucida Grande', Tahoma, Verdana, sans-serif;
    font-size: 14px;
    margin: 5px;
    margin-top: -5px;
    padding: 0 10px;
    width: 280px;
    height: 34px;
    color: #404040;
    background: white;
    border: 1px solid;
    border-color: #c4c4c4 #d1d1d1 #d4d4d4;
    border-radius: 2px;
    outline: 5px solid #eff4f7;
    -moz-outline-radius: 3px;
    -webkit-box-shadow: inset 0 1px 3px rgba(0, 0, 0, 0.12);
    box-shadow: inset 0 1px 3px rgba(0, 0, 0, 0.12);
}

.form-chgpasswd input[type=text]:focus, .form-chgpasswd input[type=password]:focus {
    border-color: #7dc9e2;
    outline-color: #dceefc;
    outline-offset: 0;
}

:-moz-placeholder {
    color: #c9c9c9 !important;
    font-size: 13px;
    }

::-webkit-input-placeholder {
    color: #ccc;
    font-size: 13px;
    }
    </style>
</head>

<body>
    <?php include "lib/includes.php"; ?>
    
    <style>
        .ui-widget{font-size:14px;}
    </style>
    
    <div style="align:center;">
    <div id="entete">
    </div>

    <div id="main">
        <div id='contenu-chgpasswd' style="font-family: 'Lucida Grande', Tahoma, Verdana, sans-serif; font-size: 14px;">
            <div class='form-chgpasswd ui-corner-all'>
                <h1 class= 'ui-widget-header' id="chg_passwd">Change password</h1>
                <form onSubmit="return false;">
                    <?php
                    if(!isset($_SESSION['change_passwd_flag'])){
                        echo "<div id=\"label_old_passwd\">";
                        echo "ancien mot de passe :";
                        echo "</div>";
                        echo "<p><input type=\"password\" name=\"passwd_old\" value=\"\" placeholder=\"old password\" id=\"old_passwd\"></p>";
                    }
                    ?>
                    <div id="label_new_passwd">
                    New password :
                    </div>
                    <p><input type="password" name="passwd1" value="" placeholder="new password" id='passwd1'></p>
                    <div id="label_confirm_passwd">
                    New password confirmation :
                    </div>
                    <p><input type="password" name="passwd2" value="" placeholder="new password confirmation" id='passwd2'></p>
                    <div style="font-size: 12px;
                                text-color:#FF0000; float:left;"
                         id='info'></div>
                    <div style="text-align:right;">
                        <input type="submit" value="Change" id='bchange'>
                        <button id='bannuler'>Cancel</button>
                    </div>
                </form>
            </div>
        </div>
    </div>
    
    <div id="piedpage">
    </div>
</body>

<?php include "controllers/change_password.php"; ?>

<script type="text/javascript" src="lib/js/strings.js"></script>
<script type="text/javascript">
jQuery(document).ready(function(){
     $.ajaxSetup({ cache: false });

//
// initialisation de l'interface
//
    // traduction
    $("#chg_passwd").text(str_chg_passwd.capitalize());
    $("#label_old_passwd").text(str_label_old_passwd.capitalize());
    $("#label_new_passwd").text(str_label_new_passwd.capitalize());
    $("#label_confirm_passwd").text(str_label_confirm_passwd.capitalize());
    $("#bchange").val(str_change.capitalize());
    $("#bannuler").text(str_cancel.capitalize())

    // chargement des sous-pages
    $("#entete").load("views/commun/login-entete.php");
    $("#piedpage").load("views/commun/page-pied.php");

    // gestion des "placeholder" pour les navigateurs compatibles
    if(jQuery.support.placeholder==false) {
        // No default treatment
        $('[placeholder]').focus(function() {
            if($(this).val()==$(this).attr('placeholder'))
                $(this).val('');
            if($(this).data('type')=='password')
                $(this).get(0).type='password';
        });
        $('[placeholder]').blur(function() {
            if($(this).val()==''){
                if($(this).attr('type')=='password') {
                    $(this).data('type','password').get(0).type='text';
                }
                $(this).val($(this).attr('placeholder'));
            }
        }).blur();
    }
    
    // associations bouton/callback
    $('#bchange')
        .button()
        .click(wrap_chgpasswd);
        
    $('#bannuler')
        .button()
        .click(function(){window.location='index.php'});

//
// fonctions de contrôle de surface
//
    function valide_password(password){
        if(password.length<=8){
            return 1;
        }
    }
    
//
// callbacks
//
    function wrap_chgpasswd() {
        passwd1=$('#passwd1').val();
        passwd2=$('#passwd2').val();
        
        // les controles de surfaces
        if(valide_password(passwd1)){
            $(info).text("8 caractères minimum").fadeIn(1000).delay(1000).fadeOut(1000);
            return 0;
        }
        if(passwd1!=passwd2){
            $(info).text("mot de passe ...").fadeIn(1000).delay(1000).fadeOut(1000);
            return 0;
        }

        // accès au contrôleur
        chgpasswd(passwd1, passwd2, $('#old_passwd').val(), '#info');
    }
});
</script>
</html>
