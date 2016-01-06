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

<!--
<style>
#editor {
    /** Setting height is also important, otherwise editor wont showup**/
    height: 98%;
}
</style>
-->

<script src="lib/ace/src-noconflict/ace.js" type="text/javascript" charset="utf-8"></script>

<script>

var editor;

function domenu(action)
{
   switch(action)
   {
      case 'new':
         editor.setValue(""); 
         break;

      case 'open':
         name="inputs.srules";
         $.get("models/get_file.php", { type: "srules", name: name }, function(response) {
            if(response.iserror==false)
            {
               editor.setValue(response.file, -1);
            }
            else
            {
               alert(response.errMsg);
            }
         });
         break;

      default:
         alert(action);
         break;
   }
}


jQuery(document).ready(function() {
   editor = ace.edit("editor"); 
   editor.setOption("fixedWidthGutter", true);
   editor.setTheme("ace/theme/xcode"); 
   editor.session.setMode("ace/mode/mearules"); 
   editor.session.setTabSize(3);
   editor.session.setUseSoftTabs(true);

   editor.setOptions({
     fontFamily: "'Lucida Console', Monaco, monospace",
   });

   $(document).on( "CenterResize", function( event, arg1, arg2 ) {
      // il faut attendre quelques ms avec de lancer le resize le temps que la taille du centre soit correctement mis en place ...
      setTimeout( function() { editor.resize(); }, 125);
   });   

   editor.setValue("", -1);

});
</script>

<!--
<div style="width:auto;height:100%;padding:10px;padding-bottom:0px;padding-top:0px">
-->
<div style="width:auto;height:100%;">
<div style="width:100%;height:100%;display:table">
<!--
      <div class="easyui-panel" style="padding:5px;">
-->
      <div style=display:table-row;">
<!--
      <div class="easyui-panel" style="width:100%; margin: 0 auto;">
      <div style="height:28px; width:100%;padding-top:6px;border-bottom:1px solid #95B8E7;">
-->
      <div style="height:28px; width:100%;padding-top:6px;border-bottom:1px solid #95B8E7;">
         <a href="#" class="easyui-menubutton" data-options="menu:'#mm1'">File</a>
<!--
         <a href="#" class="easyui-menubutton" data-options="menu:'#mm2',iconCls:'icon-edit'">Edit</a>
-->
      </div>
      <div id="mm1" style="width:150px;">
         <div onclick="javascript:domenu('new')">New</div>
         <div onclick="javascript:domenu('open')">Open</div>
         <div onclick="javascript:domenu('save')">Save</div>
         <div onclick="javascript:domenu('saveas')">Save as</div>
         <div>Close</div>
      </div>
      </div>
<!--
<div id="mm2" style="width:150px;">
   <div data-options="iconCls:'icon-undo'" onclick="javascript:domenu('undo')">Undo</div>
   <div data-options="iconCls:'icon-redo'" onclick="javascript:domenu('redo')">Redo</div>
   <div class="menu-sep"></div>
   <div onclick="javascript:domenu('cut')">Cut</div>
   <div onclick="javascript:domenu('copy')">Copy</div>
   <div onclick="javascript:domenu('paste')">Paste</div>
   <div class="menu-sep"></div>
   <div onclick="javascript:domenu('selectall')">Select All</div>
</div>
-->
      <div id="editor" style="width:100%;height:100%;display:table-row;">
<!--
            <pre id="editor"></pre> 
-->
      </div>
</div>
</div>

