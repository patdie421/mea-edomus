<?php
include_once('lib/configs.php');
include_once('lib/php/translation.php');
include_once('lib/php/$LANG/translation.php');
mea_loadTranslationData($LANG,'');
session_start();
?>

<!DOCTYPE html>

<html>
<head>
   <title>
   <?php echo $TITRE_APPLICATION;?>
   </title>
   <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
   <meta name="viewport" content="width=device-width, initial-scale=0.99">
   <meta name="description" content="domotique DIY !">

   <link rel="stylesheet" type="text/css" href="lib/jquery-easyui-1.4.4/themes/default/easyui.css">
   <link rel="stylesheet" type="text/css" href="lib/jquery-easyui-1.4.4/themes/icon.css">
   <link rel="stylesheet" type="text/css" href="lib/jquery-easyui-1.4.4/themes/color.css">
   <link rel="stylesheet" type="text/css" href="lib/mea-edomus.css">
   
   <script type="text/javascript" src="lib/jquery-easyui-1.4.4/jquery.min.js"></script>
   <script type="text/javascript" src="lib/jquery-easyui-1.4.4/jquery.easyui.min.js"></script>
</head>

<script type="text/javascript" src="models/common/models-utils.js"></script>

<script type="text/javascript" src="controllers/common/meaobject.js"></script>
<script type="text/javascript" src="controllers/common/commoncontroller.js"></script>
<script type="text/javascript" src="controllers/common/translationcontroller.js"></script>
<script type="text/javascript" src="controllers/common/credentialcontroller.js"></script>

<!-- surcharge des méthodes du controleur de traduction spécifiques à une langue donnée -->
<?php
   echo "<script type='text/javascript' src='lib/js/".$LANG."/mea-translation.js'></script>"; // extension
?>

<script type="text/javascript" src="controllers/password-ctrl.js"></script>

<script type="text/javascript">
<?php echo "LANG='$LANG';"; ?>
var chgPasswordDest="";
var destination="";


function login()
{
   passwordController.login($("#userid").val(),$("#passwd").val(),destination,chgPasswordDest);
}


jQuery(document).ready(function(){
   $.ajaxSetup({ cache: false });

   translationController = new TranslationController();
   translationController.loadDictionaryFromJSON("lib/translation_"+LANG+".json");
   extend_translation(translationController);
   
   // controleur d'habilitation
   credentialController = new CredentialController("models/get_auth.php");

   passwordController = new PasswordController('models/login.php','models/set_password.php');
   passwordController.linkToTranslationController(translationController);
   passwordController.linkToCredentialController(credentialController);

   userid = $("#userid");
   passwd = $("#passwd");
  
   userid.textbox('textbox').on('keydown', function(e) {
      if(e.keyCode == 13)
         passwd.textbox('textbox').focus();
   });

   passwd.textbox('textbox').on('keydown', function(e) {
      if(e.keyCode == 13)
         login();
   });

   <?php
      echo "chgPasswordDest = \"change_password2.php";
      if(isset($_REQUEST['view'])) {
         $view=$_REQUEST['view'];
         echo "?view=$view";
      }
      echo "\";\n";
      
      echo "destination = \"index.php";
      if(isset($_REQUEST['view']))
      {
         echo "?view="; echo $_REQUEST['view'];
      }
      echo "\";\n";
   ?>

   $('#login').click(login);
});
</script>

<body>
   <div style="min-width:950px;min-height:650px;margin:10px;text-align:center;">
      <div>
         <div style="display:inline-block">
            <img src="lib/logo-mea-eDomus.png" border="0" align="center" width=500px/>
         </div>
      </div>
      <div style="display:inline-block;margin-top:75px">
         <div class="easyui-panel" title="<?php mea_toLocalC('Login to system'); ?>" style="width:400px;padding:30px 70px 20px 70px;margin:0 auto">
            <div style="margin-bottom:10px">
               <input id="userid" class="easyui-textbox" autocapitalize="off" autocorrect="off" style="width:100%;height:40px;padding:12px;" data-options="prompt:'<?php mea_toLocalC('user id'); ?>',iconCls:'icon-man',iconWidth:38">
            </div>
            <div style="margin-bottom:20px">
               <input id="passwd" class="easyui-textbox" type="password" style="width:100%;height:40px;padding:12px" data-options="prompt:'<?php mea_toLocalC('password'); ?>',iconCls:'icon-lock',iconWidth:38">
            </div>
            <a href="#" id="login" class="easyui-linkbutton" data-options="iconCls:'icon-ok'" style="padding:5px 0px;width:100%;">
               <span style="font-size:14px;"><?php mea_toLocalC('login'); ?></span>
            </a>
         </div>
      </div>
   </div>
</body>
</html>
