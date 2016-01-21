<?php
include_once('../lib/configs.php');
include_once('../lib/php/translation.php');
include_once('../lib/php/$LANG/translation.php');
include_once('../lib/php/auth_utils.php');
mea_loadTranslationData($LANG,'../');

session_start();

$isadmin = check_admin();
if($isadmin !=0 && $isadmin != 98) : ?>
<script>
   window.location = "login.php";
</script>
<?php
   exit(1);
endif;
?>

<script type="text/javascript" src="controllers/ruleseditorcontroller.js"></script>

<script>
jQuery(document).ready(function() {
   ctrlr_rulesEditor = new RulesEditorController(
      "myeditorzone",
      "myeditor",
      "myeditormenu");
   ctrlr_rulesEditor.linkToTranslationController(translationController);
   ctrlr_rulesEditor.linkToCredentialController(credentialController);

   domenu_re = ctrlr_rulesEditor.domenu.bind(ctrlr_rulesEditor);

   ctrlr_rulesEditor.start();
});
</script>


<div id="myeditorzone" style="width:auto;height:100%;">
   <div id="myeditormenu" style="height:28px; width:100%;padding-top:6px;border-bottom:1px solid #95B8E7;display:none">
      <a href="#" class="easyui-menubutton" data-options="menu:'#mm1'"><?php mea_toLocalC('rules'); ?></a>
      <div id="mm1" style="width:150px; display:none">
         <div onclick="javascript:domenu_re('new')"><?php mea_toLocalC('new'); ?></div>
         <div onclick="javascript:domenu_re('open')"><?php mea_toLocalC('open'); ?></div>
         <div onclick="javascript:domenu_re('save')"><?php mea_toLocalC('save'); ?></div>
         <div onclick="javascript:domenu_re('saveas')"><?php mea_toLocalC('save as'); ?></div>
         <div class="menu-sep"></div>
         <div onclick="javascript:domenu_re('delete')"><?php mea_toLocalC('delete'); ?></div>
      </div>
   </div>
   <div id="myeditor" style="width:100%;height:0px;display:none"></div>
</div>
