<?php
include_once('lib/configs.php');
include_once('lib/php/translation.php');
include_once('lib/php/$LANG/translation.php');
mea_loadTranslationData($LANG,'');
session_start();
?>

<script>
jQuery(document).ready(function(){
<?php
   if(isset($_REQUEST['view'])) {
      echo "var destview=\""; echo $_REQUEST['view']; echo "\";\n";
   }
   else {
      echo "var destview=\"\";\n";
   }
?>
   if(destview=="")
      destview=translationController.toLocalC('rules editor');
   else
      destview=destview.mea_hexDecode();

   page4_ctrlr = new TabsPageController("page4_tab");
   page4_ctrlr.linkToTranslationController(translationController);
   page4_ctrlr.linkToCredentialController(credentialController); // pour la gestion des habilitations
   page4_ctrlr.start(destview);

   s=liveComController.getSocketio();
   if(s!=null) {
      s.removeAllListeners('aut');
   }
   else {
      windows.location = "index.php"; // pas de com live, cette page ne devrait pas s'afficher, on recharge l'index
   }
});
</script>

<div id="page4_tab" class="easyui-tabs" border=false fit=true>
    <div id="<?php mea_toLocalC('rules manager'); ?>" title="<?php mea_toLocalC('rules manager'); ?>" href="views/rulesmanager-view.php"></div>
    <div id="<?php mea_toLocalC('rules editor'); ?>" title="<?php mea_toLocalC('rules editor'); ?>" href="views/ruleseditor-view.php"></div>
    <div id="<?php mea_toLocalC('map editor'); ?>" title="<?php mea_toLocalC('map editor'); ?>" href="views/mapeditor-view.php"></div>
</div>
