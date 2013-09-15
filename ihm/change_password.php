<?php
session_start();
?>
<!DOCTYPE html>
<?php

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
    <title>MEA eDomus</title>
    <meta charset="utf-8">
    <style>
.form-chgpasswd {
    position: relative;
    margin: 0 auto;
    margin-top:50px;
    padding: 20px 20px 20px;
    width: 310px;

    border-color:black;
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
    color: #555;
    text-align: center;
    text-shadow: 0 1px white;
    background: #f3f3f3;
    border-bottom: 1px solid #cfcfcf;
    border-radius: 3px 3px 0 0;
    background-image: -webkit-linear-gradient(top, whiteffd, #eef2f5);
    background-image: -moz-linear-gradient(top, whiteffd, #eef2f5);
    background-image: -o-linear-gradient(top, whiteffd, #eef2f5);
    background-image: linear-gradient(to bottom, whiteffd, #eef2f5);
    -webkit-box-shadow: 0 1px whitesmoke;
    box-shadow: 0 1px whitesmoke;
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

.form-chgpasswd input[type=text]:focus, .form-login input[type=password]:focus {
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
    <?php include "lib/includes.php";?>
    <style>
        .ui-widget{font-size:14px;}
    </style>
    
    <div style="align:center;">
    <div id="entete">
    </div>

    <div id="main">
        <div id='contenu-chgpasswd' style="font-family: 'Lucida Grande', Tahoma, Verdana, sans-serif; font-size: 14px;">
            <div class='form-chgpasswd'>
                <h1>Change password</h1>
                <form onSubmit="return false;">
                    <?php
                    if(!isset($_SESSION['change_passwd_flag'])){
                        echo "<div>";
                        echo "ancien mot de passe :";
                        echo "</div>";
                        echo "<p><input type=\"password\" name=\"passwd_old\" value=\"\" placeholder=\"old password\" id=\"old_passwd\"></p>";
                    }
                    ?>
                    <div>
                    nouveau mot de passe :
                    </div>
                    <p><input type="password" name="passwd1" value="" placeholder="new password" id='passwd1'></p>
                    <div>
                    confirmation :
                    </div>
                    <p><input type="password" name="passwd2" value="" placeholder="new password confirmation" id='passwd2'></p>
                    <div style="font-size: 12px;
                                text-color:#FF0000; float:left;"
                         id='info'></div>
                    <div style="text-align:right;">
                        <input type="submit" value="changer" id='blogin'>
                        <button id='bannuler'>Annuler</button>
                    </div>
                </form>
            </div>
        </div>
    </div>
    
    <div id="piedpage">
    </div>

    <div id="dialog-confirm" title="Default" style="display:none;">
        <p><span class="ui-icon ui-icon-alert" style="float: left; margin: 0 7px 20px 0;"></span><div id="dialog-confirm-text">Default<div></p>
    </div>

    <script type="text/javascript">

$(function(){
    function valide_password(password){
        if(password.length<=8){
            return 1;
        }
    }
    
    function mea_alert(title, text, fx){
        $("#dialog-confirm-text").html(text);
        $( "#dialog-confirm" ).dialog({
            title: title,
            resizable: true,
            height:250,
            width:500,
            modal: true,
            buttons: {
                Ok: function() {
                    $( this ).dialog( "close" );
                    fx();
                }
            }
        });
    }

    function chgpasswd(){
        passwd1=$('#passwd1').val();
        passwd2=$('#passwd2').val();
        
        if(valide_password(passwd1)){
            $('#info').text("8 caractères minimum").fadeIn(1000).delay(1000).fadeOut(1000);
            return 0;
        }
        
        if(passwd1!=passwd2){
            $('#info').text("mot de passe ...").fadeIn(1000).delay(1000).fadeOut(1000);
            return 0;
        }

        if($('#old_passwd').length)
            old_password=$('#old_passwd').val();
        
        <?php
            if(!isset($_SESSION['change_passwd_flag'])){
                echo "set_password_params={old_password:old_password, new_password:passwd1};";
            }else{
                echo "set_password_params={new_password:passwd1};";
            }
         ?>

        $.ajax({ url: 'models/set_passwd.php',
            async: false,
            type: 'GET',
            dataType: 'json',
            data: set_password_params,
            success: function(data){
                if(data.retour==1)
                {
                    mea_alert('Succes','Le mot de passe a été changé avec succès.',function(){
                    <?php
                        if(isset($_GET['dest'])){
                            $dest=$_GET['dest'];
                            echo "window.location = \"$dest\";";
                        }else{
                            echo "window.location = \"index.php\";";
                        }
                    ?>
                });
                }
                else
                    $('#info').text("mot de passe incorrecte").delay(3000).fadeOut(1000);
            },
            error:function(){
                $('#info').text("Erreur ...").delay(3000).fadeOut(1000);
            }
        });
    }

    $.ajaxSetup({ cache: false });

    $("#entete").load("views/commun/login-entete.php");
    $("#piedpage").load("views/commun/page-pied.php");

    if(jQuery.support.placeholder==false){
        // No default treatment
        $('[placeholder]').focus(function(){
            if($(this).val()==$(this).attr('placeholder'))
                $(this).val('');
            if($(this).data('type')=='password')
                $(this).get(0).type='password';
        });
        $('[placeholder]').blur(function(){
            if($(this).val()==''){
                if($(this).attr('type')=='password'){
                    $(this).data('type','password').get(0).type='text';
                }
                $(this).val($(this).attr('placeholder'));
            }
        }).blur();
    }
    
    $('#blogin')
        .button()
        .click(chgpasswd);
    $('#bannuler')
        .button()
        .click(function(){window.location='index.php'});
});
    </script>
</body>
</html>
