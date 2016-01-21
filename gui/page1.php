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
      destview=translationController.toLocalC('indicators');
   else
      destview=destview.mea_hexDecode();

   s=liveComController.getSocketio();
   if(s!=null) {
      s.removeAllListeners('mon');
      s.removeAllListeners('rel');
   }
   else {
      windows.location = "index.php"; // pas de com live, cette page ne devrait pas s'afficher, on recharge l'index
   }
   
   page1_ctrlr = new TabsPageController("page1_tab");
   page1_ctrlr.linkToTranslationController(translationController);
   page1_ctrlr.linkToCredentialController(credentialController); // pour la gestion des habilitations
   page1_ctrlr.start(destview);
});
</script>

<div id="page1_tab" class="easyui-tabs" border=false fit=true>
    <div title="<?php mea_toLocalC('indicators'); ?>" href="views/indicateurs-view.php" style="padding:20px;"></div>
    <div title="<?php mea_toLocalC('services'); ?>" href="views/services-view.php" style="padding:20px;"></div>
</div>
