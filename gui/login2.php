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
    <?php echo $TITRE_APPLICATION;?>
    </title>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=0.99">
</head>

<body>
   <div style="min-width:950px;min-height:650px;margin:10px;">
      <div class="easyui-panel" title="Login to system" style="width:400px;padding:30px 70px 20px 70px">
         <div style="margin-bottom:10px">
            <input id="userid" class="easyui-textbox" style="width:100%;height:40px;padding:12px" data-options="prompt:'Username',iconCls:'icon-man',iconWidth:38">
         </div>
         <div style="margin-bottom:20px">
            <input id="passwd" class="easyui-textbox" type="password" style="width:100%;height:40px;padding:12px" data-options="prompt:'Password',iconCls:'icon-lock',iconWidth:38">
         </div>
         <div style="margin-bottom:20px">
            <input type="checkbox" checked="checked">
         </div>
         <a href="#" class="easyui-linkbutton" data-options="iconCls:'icon-ok'" style="padding:5px 0px;width:100%;">
            <span style="font-size:14px;">Login</span>
         </a>
      </div>
   </div>
</body>

<script type="text/javascript">
jQuery(document).ready(function(){
   $.ajaxSetup({ cache: false });

   function login(){
      if($("#userid").val()!="")
      {
         $.get('models/auth.php',
            { user_password: $('#passwd').val(),
                  user_name: $('#userid').val()
            },
            function(data) {
               if(data.retour==1) {
                  if(data.flag==1) {
                     $.messager.alert("Première Connexion ...","Le mot de passe doit être changé", 'info', function(){
                     <?php
                        echo "window.location = \"change_password.php";
                        if(isset($_GET['dest'])){
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
                  alert("erreur identifiant/mdp (à remlacer)"; );
               }
            },
            "json"
         )
         .fail(function(){ alert("erreur de com. (à remplacer)"; });
      }
      else {
         alert("userid ne peut pas être vide (à remplacer)";
      }
   }
});

</script>
</html>

