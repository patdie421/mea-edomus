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
      destview=translationController.toLocalC('ruleseditor');

   page1_ctrlr = new TabsPageController("page4_tab");
   page1_ctrlr.linkToTranslationController(translationController);
   page1_ctrlr.linkToCredentialController(credentialController); // pour la gestion des habilitations
   
   page1_ctrlr.start(destview);
});
</script>

<div id="page4_tab" class="easyui-tabs" border=false fit=true>
    <div title="<?php mea_toLocalC('rules editor'); ?>" href="views/ruleseditor-view.php" style="padding:20px;"></div>
</div>
