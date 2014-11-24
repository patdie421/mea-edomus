<?php
include "lib/configs.php";
//
//  PAGE PRINCIPALE (VIEW) : connexion à l'application
//
session_start();

ob_start();
print_r($_REQUEST);
$debug_msg = ob_get_contents();
ob_end_clean();
error_log($debug_msg);

?>
<!DOCTYPE html>
<html>
<head>
   <title>
   <?php echo $TITRE_APPLICATION;?>
   </title>
   <meta charset="utf-8">
   <meta name="viewport" content="width=device-width, initial-scale=0.99">

   <link rel="stylesheet" type="text/css" href="lib/jquery-easyui-1.4.1/themes/default/easyui.css">
   <link rel="stylesheet" type="text/css" href="lib/jquery-easyui-1.4.1/themes/icon.css">
   <link rel="stylesheet" type="text/css" href="lib/jquery-easyui-1.4.1/themes/color.css">
   <link rel="stylesheet" type="text/css" href="lib/mea-edomus.css">
   
   <script type="text/javascript" src="lib/jquery-easyui-1.4.1/jquery.min.js"></script>
   <script type="text/javascript" src="lib/jquery-easyui-1.4.1/jquery.easyui.min.js"></script>
</head>

<body>
   <div style="min-width:950px;min-height:650px;margin:10px;text-align:center;">
      <div>
         <div style="display:inline-block">
            <img src="lib/logo-mea-eDomus.png" border="0" align="center" width=500 height=100%/>
         </div>
      </div>
      <div style="display:inline-block;margin-top:75px">
         <div class="easyui-panel" title="Login to system" style="width:400px;padding:30px 70px 20px 70px;margin:0 auto">
            <div style="margin-bottom:10px">
               <input id="userid" class="easyui-textbox" style="width:100%;height:40px;padding:12px" data-options="prompt:'Nom',iconCls:'icon-man',iconWidth:38">
            </div>
            <div style="margin-bottom:20px">
               <input id="passwd" class="easyui-textbox" type="password" style="width:100%;height:40px;padding:12px" data-options="prompt:'Mot de passe',iconCls:'icon-lock',iconWidth:38">
            </div>
            <a href="#" id="login" class="easyui-linkbutton" data-options="iconCls:'icon-ok'" style="padding:5px 0px;width:100%;">
               <span style="font-size:14px;">Login</span>
            </a>
         </div>
      </div>
   </div>
</body>

<script type="text/javascript">
jQuery(document).ready(function(){
   $.ajaxSetup({ cache: false });

   function login(){
      if($("#userid").val()!="") {
         $.get('models/auth.php', {
                  user_password: $('#passwd').val(),
                  user_name: $('#userid').val()
               },
               function(data) {
                  if(data.retour==1) {
                     if(data.flag==1) {
                     $.messager.alert("Première Connexion ...","Le mot de passe doit être changé !", 'info', function(){
                        <?php
                           echo "window.location = \"change_password.php";
                           if(isset($_GET['dest'])) {
                              $dest=$_GET['dest'];
                              echo "?dest=$dest";
                           }
                           echo "\";";
                        ?>
                        });
                     }
                     else {
                        <?php
                           echo "window.location = ";
                           if(isset($_GET['dest'])){
                              echo "\""; echo $_GET['dest'];
                           }
                           else {
                              echo "\""; echo "index.php";
                           }
                           echo "\";";
                        ?>
                     }
                  }
                  else {
                     $.messager.alert("Connexion impossible ...","nom et/ou mot de passe incorrecte !", 'error');
                  }
               }
         ).fail(function(){ $.messager.alert("Connexion impossible ...","erreur de communication avec le serveur !", 'error'); });
      }
      else {
         $.messager.alert("Connexion impossible ...","nom et/ou mot de passe incorrecte !", 'error');
      }
   }
   
   $('#login').click(login);
});
</script>
</html>

