<?php
include_once('lib/configs.php');
include_once('lib/php/auth_utils.php');

session_start();
if(isset($_SESSION['language']))
{
   $LANG=$_SESSION['language'];
}
include_once('lib/php/translation.php');
include_once('lib/php/$LANG/translation.php');
mea_loadTranslationData($LANG,'');

$isadmin = check_admin();
?>

<script>
jQuery(document).ready(function(){
/*
function page5leaveCallback()
{
   var _this = this;
   for(var cb in _this.leaveViewsCallbacks)
   {
      cb(); 
   }
}
*/
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

   page5_ctrlr = new TabsPageController("page5_tab");
   page5_ctrlr.linkToTranslationController(translationController);
   page5_ctrlr.linkToCredentialController(credentialController); // pour la gestion des habilitations
   page5_ctrlr.start(destview);

   // pour l'installation des callbacks
   page5_ctrlr.leaveViewsCallbacks = [];

   page5_ctrlr.leaveCallback = function page5leaveCallback()
   {
      for(var i in this.leaveViewsCallbacks) {
         this.leaveViewsCallbacks[i]();
      }
      $("#page5_tab").remove();
   }
   page5_ctrlr.addLeaveViewsCallbacks = function(cb) {
      this.leaveViewsCallbacks.push(cb);
   };
   var page5leaveCallback = page5_ctrlr.leaveCallback.bind(page5_ctrlr);

   viewsController.removePageChangeCallback("page5.php");
   viewsController.addPageChangeCallback("page5.php", page5leaveCallback);
   // fin callbacks

   s=liveComController.getSocketio();
   if(s!=null) {
      s.removeAllListeners('aut');
   }
   else {
      windows.location = "index.php"; // pas de com live, cette page ne devrait pas s'afficher, on recharge l'index
   }
});
</script>

<div id="page5_tab" class="easyui-tabs" border=false fit=true>
<?php
if($isadmin==0) :?>
    <div id="<?php mea_toLocalC('map editor'); ?>" title="<?php mea_toLocalC('map editor'); ?>" href="views/mapeditor-view.php"></div>
    <div id="<?php mea_toLocalC('maps set editor'); ?>" title="<?php mea_toLocalC('maps set editor'); ?>" href="views/mapsseteditor-view.php"></div>
<?php
endif
?>
</div>
