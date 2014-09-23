<?php
include "lib/configs.php";
//
//  PAGE PRINCIPALE (VIEW) : connexion à l'application
//
session_start()
?>
<!DOCTYPE html>
<html>
<head>
    <title>
    <?php echo $TITRE_APPLICATION; ?>
    </title>
    <meta charset="utf-8">
    <style>
.form-login {
    position: relative;
    margin: 0 auto;
    margin-top:50px;
    padding: 20px 20px 20px;
    width: 310px;

    border-color:#a6c9e2;
    border-width:1px;
    border-style:solid;
    }
    
.form-login p.submit {
    text-align:center;}

.form-login h1 {
    font-family: 'Lucida Grande', Tahoma, Verdana, sans-serif;
    font-size: 18px;
    margin: -20px -20px 21px;
    line-height: 40px;
    font-size: 15px;
    font-weight: bold;
    text-align: center;
    }

.form-login input[type=text], .form-login input[type=password] {
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

.form-login input[type=text]:focus, .form-login input[type=password]:focus {
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
        <div id='contenu-login' style="font-family: 'Lucida Grande', Tahoma, Verdana, sans-serif; font-size: 14px;">
            <div class='form-login ui-corner-all'>
                <h1 class= 'ui-widget-header' id="login">Login</h1>    
                <form onSubmit="return false;">
                    <div id="label_userid">
                    id :
                    </div>
                    <p><input type="text" name="userid" value="" placeholder="userid" id='userid'></p>
                    <div id="label_passwd">
                    password :
                    </div>
                    <p><input type="password" name="password" value="" placeholder="Password" id='passwd'></p>
                    <div style="font-size: 12px;
                                text-color:#FF0000; float:left;"
                         id='info'></div>
                    <div style="text-align:right;"><input type="submit" value="login" id='blogin'></div>
                </form>
            </div>
        </div>
    </div>
    
    <div id="piedpage">
    </div>
</body>

<script type="text/javascript" src="lib/js/strings.js"></script>
<script type="text/javascript">
jQuery(document).ready(function(){
    $.ajaxSetup({ cache: false });

    $("#login").text(str_title_login.capitalize());
    $("#label_userid").text(str_label_userid.capitalize());
    $("#label_passwd").text(str_label_passwd.capitalize());
    $("#blogin").val(str_connection.capitalize());
    
    function login(){
        if($("#userid").val()!="")
        {
            $.get('models/auth.php',
                { user_password: $('#passwd').val(),
                  user_name: $('#userid').val() },
                function(data){
                    if(data.retour==1){
                        if(data.flag==1){
                            mea_alert2(str_first_login,str_passwd_must_be_changed,function(){
                            <?php
                                echo "window.location = \"change_password.php";
                                if(isset($_GET['dest'])){
                                    $dest=$_GET['dest'];
                                    echo "?dest=$dest";
                                }
                                echo "\";";
                            ?>
                            });
                        }else{
                        <?php
                            echo "window.location = ";
                            if(isset($_GET['dest'])){
                                echo "\""; echo $_GET['dest'];
                            }else{
                                echo "\""; echo "index.php";
                            }
                            echo "\";";
                        ?>
                        }
                    }else{
                        $('#info').text(data.erreur).fadeIn(1000).delay(1000).fadeOut(1000);
                    }
                },
                "json"
            )
            .fail(function(){$('#info').text(str_not_com).fadeIn(1000).fadeOut(1000);});
        }else{
            $('#info').text(str_id_not_null).fadeIn(1000).delay(1000).fadeOut(1000);
        }
    }

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
            .click(login);

});
</script>
</html>
