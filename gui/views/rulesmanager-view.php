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

<script type="text/javascript" src="controllers/common/filechoosercontroller.js"></script>

<script>
var current_file = false;

function load_select(_selectid, after_load, on_error)
{
   _type="srules";
   $.get("models/get_files_list.php", { type: _type }, function(response) {
      if(response.iserror === false)
      {
         $('#'+_selectid).empty();
         for(var i in response.values)
            $('#'+_selectid).append(new Option(response.values[i].slice(0, -(_type.length+1)), i));
         after_load();
      }
      else
      {
         on_error();
      }
   });
}


function save_rules_set(name, type, checkflag)
{
   var data=[];

   $('#files_sel_rm option').each(function() {
      data.push($(this).text());
   });
 
   $.post("models/put_file.php", { type: type, name: name, file: JSON.stringify(data) }, function(response) {
   if(response.iserror===false)
      current_file=name;
   else
      $.messager.alert('Error', "Can't save file ! (server msg: "+response.errMsg+")", 'error');
   });
}


function load_rules_set(name, type, checkflag)
{
   __load_rules_set = function(name, type, checkflag)
   {
      current_file=name;
      if(checkflag === true)
         return;

      $.get("models/get_file.php", { type: type, name: name }, function(response) {
         if(response.iserror===false)
         {
            var data = JSON.parse(response.file);
            var notfound = [];
            for (var i in data)
            {
               var found = false;
               $('#files_to_sel_rm option').each(function() {
                  if($(this).text()===data[i])
                  {
                     id=$(this).val();
                     $(this).remove();
                     $('#files_sel_rm').append(new Option(data[i], id));
                     found = true;
                     return;
                  }
               });
               if (found===false)
                  notfound.push(data[i]);
            }
            if(notfound.length != 0)
               $.messager.alert('Error', 'rules(s) not found: '+JSON.stringify(notfound), 'error');
         }
         else
         {
         }
      });
   };

   $('#files_to_sel_rm').empty();
   $('#files_sel_rm').empty();
   load_select("files_to_sel_rm", function() { __load_rules_set(name, type, checkflag); }, function() { alert("error"); } );
}


function domenu_rm(action)
{
   switch(action)
   {
      case 'new':
         current_file=false;
         $('#files_sel_rm').empty();
         load_select('files_to_sel_rm', function() {}, function() {});
         break;
      case 'save':
         if(current_file!=false)
            save_rules_set(current_file, "rset", false);
         else
            ctrlr_filechooser.open("Choose rules set ...", "Save as:", "Save", "Cancel", "rset", true, true, "file exist, overhide it ?", load_rules_set);
         break;

      case 'load':
         ctrlr_filechooser.open("Choose rules set ...", "Load:", "Load", "Cancel", "rset", true, true, "file does not exist, new file ?", load_rules_set);
         break;

      case 'saveas':
         ctrlr_filechooser.open("Choose rules set ...", "Save as:", "OK", "Annuler", "rset", true, false, "file exist, overhide it ?", save_rules_set);
         break;
   }
}


jQuery(document).ready(function() {
   ctrlr_filechooser = new FileChooserController();


   load_select("files_to_sel_rm", function() {}, function() {});

   $('#button_in_rm').click(function() {  
    return !$('#files_to_sel_rm option:selected').remove().removeAttr("selected").appendTo('#files_sel_rm');  
   });  
   $('#button_out_rm').click(function() {  
    return !$('#files_sel_rm option:selected').remove().appendTo('#files_to_sel_rm');  
      selected.removeAttr("selected");
   }); 

   $('#button_up_rm').click(function() {  
      var selected = $('#files_sel_rm option:selected');
      var before = selected.prev();
      if (before.length > 0)
         selected.detach().insertBefore(before);
   }); 
   $('#button_down_rm').click(function() {  
      var selected = $('#files_sel_rm option:selected');
      var next = selected.next();
      if (next.length > 0)
         selected.detach().insertAfter(next);
   });

   $("#rulemgrmenu").show();
});
</script>

<style>
.editable_ap {
}
</style>

<div id="rulemgrzone" style="width:auto;height:100%;">
   <div id="rulemgrmenu" style="height:28px; width:100%;padding-top:6px;border-bottom:1px solid #95B8E7;display:none">
      <a href="#" class="easyui-menubutton" data-options="menu:'#mm_rm'">Rules set</a>

<div id="mm_rm" style="width:150px; display:none">
      <div onclick="javascript:domenu_rm('new')">New</div>
      <div onclick="javascript:domenu_rm('load')">Load</div>
      <div onclick="javascript:domenu_rm('save')">Save</div>
      <div onclick="javascript:domenu_rm('saveas')">Save as</div>
      <div class="menu-sep"></div>
      <div onclick="javascript:domenu_rm('delete')">Delete</div>
   </div>
</div>
<div style="margin-top:30px">
<form id="fm_rm" method="post" novalidate>
    <div style="width:100%;text-align:center;">
        
        <div id="rm" class="easyui-panel" data-options="style:{margin:'0 auto'}" title="<?php mea_toLocalC('Choose rules to build:'); ?>" style="width:700px;padding:10px;">
            <table width="600px" align="center" style="padding-bottom: 12px;">
                <col width="40%">
                <col width="10%">
                <col width="40%">
                <col width="10%">

                <tr>
                <td align="center"><B><?php mea_toLocalC('Available'); ?></B></td>
                <td></td>
                <td align="center"><B><?php mea_toLocalC('Selected'); ?></B></td>
                <td></td>
                </tr>

                <tr>
                    <td align="center">
                       <select multiple name="files_to_sel_rm" id="files_to_sel_rm" size="15" style="width:100%;font-family:verdana,helvetica,arial,sans-serif;font-size:12px;">
                       </select>
                    </td>
                    <td align="center">
                       <table>
                       <tr><td>
                       <a id="button_in_rm" href="javascript:void(0)" data-options="iconCls:'icon-mearightarrow'" class="easyui-linkbutton" style="width:30px;height:30px"></a>
                       </td></tr>
                       <tr><td>
                       <a id="button_out_rm" href="javascript:void(0)" data-options="iconCls:'icon-mealeftarrow'" class="easyui-linkbutton" style="width:30px;height:30px"></a>
                       </td></tr>
                       </table>
                    </td>
                    <td align="center">
                       <select name="files_sel_rm" id="files_sel_rm" size="15" style="width:100%;font-family:verdana,helvetica,arial,sans-serif;font-size:12px;">
                       </select>
                    </td>
                    <td align="center">
                       <table>
                       <tr><td>
                       <a id="button_up_rm" href="javascript:void(0)" data-options="iconCls:'icon-meauparrow'" class="easyui-linkbutton" style="width:30px;height:30px"></a>
                       </td></tr>
                       <tr><td>
                       <a id="button_down_rm" href="javascript:void(0)" data-options="iconCls:'icon-meadownarrow'" class="easyui-linkbutton" style="width:30px;height:30px"></a>
                       </td></tr>
                       </table>
                    </td>
                    <td align="center">
                </tr>
            </table>
        </div>

       <div style="margin:0 auto; width:700px; padding-top:20px; text-align:center;">
           <a href="javascript:void(0)" class="easyui-linkbutton" style="width=50px;" data-options="iconCls:'icon-reload'"><?php mea_toLocalC('build'); ?></a>
<?php
if($isadmin==0) : ?>
           <a href="javascript:void(0)" class="easyui-linkbutton" style="width=50px;" data-options="iconCls:'icon-meacompiler'"><?php mea_toLocalC('apply'); ?></a>
<?php
endif; ?>
       </div>
   </div>
</form>
</div>
</div>
