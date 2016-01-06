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
      setTimeout( function() {
         $("#editor").height($("#editorzone").height()-35);
         editor.resize();
      },
      25);
   });

   editor.setValue("", -1);
   $("#editor").height($("#editorzone").height()-35);
   editor.resize();
   $("#editormenu").show();
   $("#editor").show();
});
</script>

<div id="editorzone" style="width:auto;height:100%;">
   <div id="editormenu" style="height:28px; width:100%;padding-top:6px;border-bottom:1px solid #95B8E7;display:none">
      <a href="#" class="easyui-menubutton" data-options="menu:'#mm1'">File</a>
   </div>
   <div id="editor" style="width:100%;height:0px;display:none"></div>
</div>

<div id="mm1" style="width:150px; display:none">
   <div onclick="javascript:domenu('new')">New</div>
      <div onclick="javascript:domenu('open')">Open</div>
      <div onclick="javascript:domenu('save')">Save</div>
      <div onclick="javascript:domenu('saveas')">Save as</div>
      <div>Close</div>
   </div>
</div>
