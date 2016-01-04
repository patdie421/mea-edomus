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

<style>
#editor {
    /** Setting height is also important, otherwise editor wont showup**/
    height: 99%;
}
</style>

<script src="lib/src-min-noconflict/ace.js" type="text/javascript" charset="utf-8"></script>

<script>
jQuery(document).ready(function(){

   var editor = ace.edit("editor"); 
   editor.setTheme("ace/theme/xcode"); 
//    editor.session.setMode("ace/mode/javascript"); 
   editor.session.setTabSize(3);
   editor.session.setUseSoftTabs(true);

   $.get("inputs.srules", function(response) {
      editor.setValue(response);
   });

});
</script>

<pre id="editor">
</pre> 
