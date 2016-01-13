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

// globals
var myeditor;
var current_file = false;


$.extend($.fn.validatebox.defaults.rules, {
   filename_validation: {
      validator: function(value, param){
         return checkRegexp( value, /^[0-9A-Za-z]*$/);
      },
      message: "caractères dans l'intervalle [a-zA-Z0-9] uniquement."
   }
});


function _openfile(e, files, _myeditor, _type, _dialogid, _selectid, _filename, _submitid)
{
   __openfile = function(e, name, files, _myeditor, _type, _dialogid, _selectid, _submitid)
   {
      choosefile_closedlg(_dialogid, _selectid, _submitid);

      $.get("models/get_file.php", { type: _type, name: name }, function(response) {
         if(response.iserror==false)
         {
            current_file=name;
            _myeditor.setValue(response.file, -1);
         }
         else
            alert(response.errMsg);
      });
   };

   e.preventDefault();
   name=$("#"+_filename).val();
   if(name==="")
      return -1;

   filename=name+"."+_type;
   if($.inArray(filename, files) == -1)
   {
      $.messager.confirm('Confirm','file does not exist, new file ?',function(r) {
         if (r) {
            choosefile_closedlg(_dialogid, _selectid, _submitid);
            current_file=name;
            _myeditor.setValue(response.file, -1);
         }
      });
   }
   else
      __openfile(e, name, files, _myeditor, _type, _dialogid, _selectid, _filename, _submitid);
}


function _savefileas(e, files, _myeditor, _type, _dialogid, _selectid, _filename, _submitid)
{
   __savefileas = function(e, name, files, _myeditor, _type, _dialogid, _selectid, _filename, _submitid)
   {
      file=myeditor.getValue();
      choosefile_closedlg(_dialogid, _selectid, _submitid);
      $.post("models/put_file.php", { type: _type, name: name, file: file }, function(response) {
         if(response.iserror===false)
            current_file=name;
         else
            $.messager.alert('Error', "Can't save file !<BR><BR>"+response.errMsg, 'error');
      });
   };
   
   e.preventDefault();
   name=$("#"+_filename).val();
   if(name==="")
      return -1;
   filename=name+"."+_type;

   if($.inArray(filename, files) != -1)
   {
      $.messager.confirm('Confirm','file exist, replace it ?',function(r) {
         if (r)
            __savefileas(e, name, files, _myeditor, _type, _dialogid, _selectid, _filename, _submitid);
      });
   }
   else
      __savefileas(e, name, files, _myeditor, _type, _dialogid, _selectid, _filename, _submitid);
}


function _deletefile(e, files, _myeditor, _type, _dialogid, _selectid, _filename, _submitid)
{
   __delete = function(e, name, files, _myeditor, _type, _dialogid, _selectid, _filename, _submitid)
   {
      choosefile_closedlg(_dialogid, _selectid, _submitid);
      $.post("models/del_file.php", { type: _type, name: name }, function(response) {
         if(response.iserror!==false)
            $.messager.alert('Error', "Can't delete file !<BR><BR>"+response.errMsg, 'error');
      });
   };
   
   e.preventDefault();
   name=$("#"+_filename).val();
   if(name==="")
      return -1;

   $.messager.confirm('Confirm','delete '+name+' ?',function(r) {
      if (r)
         __delete(e, name, files, _myeditor, _type, _dialogid, _selectid, _filename, _submitid);
   });
}


function savefile(_type, name, file)
{
   $.post("models/put_file.php", { type: _type, name: name, file: file }, function(response) {
      if(response.iserror===false)
      {
      }
      else
         $.messager.alert('Error', "Can't save file !<BR><BR>"+response.errMsg, 'error');
   });
}


function choosefile_closedlg(_dialogid, _selectid, _submitid)
{
   $('#'+_dialogid).dialog('close');
   $('#'+_selectid).off('dblclick');
   $('#'+_selectid).off('change');
   $('#'+_submitid).off('click');
}


function choosefile(_myeditor, _type, _dialogid, _formid, _titleid, _titletext, _selectid, _filename, _submitid, _submittext, _do)
{
   $.get("models/get_files_list.php", { type: _type }, function(response) {
      if(response.iserror === false)
      {
         $("#"+_filename).textbox('setValue', "");
         $('#'+_selectid).empty();
         for(var i in response.values)
            $('#'+_selectid).append(new Option(response.values[i].slice(0, -(_type.length+1)), i));
         $('#'+_submitid).linkbutton({'text': _submittext});
         $('#'+_titleid).text(_titletext);

         $('#'+_dialogid).dialog('open');

         $('#'+_selectid).on('dblclick', function(e) {
            if(this.value !== false)
            {
               $("#"+_filename).textbox('setValue', response.values[this.value].slice(0, -(_type.length+1)));
               _do(e, response.values, _myeditor, _type, _dialogid, _selectid, _filename, _submitid);
            }
         });
         $('#'+_submitid).on('click', function(e) {
            if(!$('#'+_formid).form('validate'))
               return -1;
            _do(e, response.values, _myeditor, _type, _dialogid, _selectid, _filename, _submitid);
         });
         $('#'+_selectid).on('change', function() {
            if(this.value !== false)
               $("#"+_filename).textbox('setValue', response.values[this.value].slice(0, -(_type.length+1)));
            else
               $("#"+_filename).textbox('setValue', "");
         });
      }
   });
}


function domenu(action)
{
   switch(action)
   {
      case 'new':
         current_file=false;
         myeditor.setValue(""); 
         break;

      case 'open':
         choosefile(myeditor, "srules", "dlg_re", "fm_re", "title_re", "open:", "selectfiles_re", "filename_re", "submit_button_re", "Open", _openfile);
         break;

      case 'saveas':
         choosefile(myeditor, "srules", "dlg_re", "fm_re", "title_re", "save as:", "selectfiles_re", "filename_re", "submit_button_re", "Save as", _savefileas);
         break;

      case 'save':
         file=myeditor.getValue();
         if(current_file!=false)
            savefile("srules", current_file, file);
         else
            choosefile(myeditor, "srules", "dlg_re", "fm_re", "title_re", "save as:", "selectfiles_re", "filename_re", "submit_button_re", "Save as", _savefileas);
         break;

      case 'delete':
         choosefile(myeditor, "srules", "dlg_re", "fm_re", "title_re", "delete:", "selectfiles_re", "filename_re", "submit_button_re", "Delete", _deletefile);
         break;
      default:
         alert(action);
         break;
   }
}


jQuery(document).ready(function() {

   myeditor = ace.edit("myeditor"); 
   myeditor.setOption("fixedWidthGutter", true);
   myeditor.setTheme("ace/theme/xcode"); 
   myeditor.session.setMode("ace/mode/mearules"); 
   myeditor.session.setTabSize(3);
   myeditor.session.setUseSoftTabs(true);
   myeditor.setOptions({
     fontFamily: "'Lucida Console', Monaco, monospace",
   });

   $(document).on( "CenterResize", function( event, arg1, arg2 ) {
      setTimeout( function() {
         $("#myeditor").height($("#myeditorzone").height()-35);
         myeditor.resize();
      },
      25);
   });

   $("#myeditor").height($("#myeditorzone").height()-35);
   myeditor.setValue("", -1);
   myeditor.resize();
   $("#myeditormenu").show();
   $("#myeditor").show();
});
</script>

<div id="myeditorzone" style="width:auto;height:100%;">
   <div id="myeditormenu" style="height:28px; width:100%;padding-top:6px;border-bottom:1px solid #95B8E7;display:none">
      <a href="#" class="easyui-menubutton" data-options="menu:'#mm1'">Rules</a>
   </div>
   <div id="myeditor" style="width:100%;height:0px;display:none"></div>
</div>

<div id="mm1" style="width:150px; display:none">
      <div onclick="javascript:domenu('new')">New</div>
      <div onclick="javascript:domenu('open')">Open</div>
      <div onclick="javascript:domenu('save')">Save</div>
      <div onclick="javascript:domenu('saveas')">Save as</div>
      <div class="menu-sep"></div>
      <div onclick="javascript:domenu('delete')">Delete</div>
   </div>
</div>


<div id="dlg_re" class="easyui-dialog" title="Choose rules to ..." style="width:350px;height:400px;padding:10px 20px" modal="true" closed="true" buttons="#dlg_buttons_re" style="display:none;">
    <div id="title_re" class="ftitle"></div>
    <form id="fm_re" method="post" data-options="novalidate:false">
        <div class="fitem">
            <select name="selectfiles_re" id="selectfiles_re" size="15" style="width:100%;font-family:verdana,helvetica,arial,sans-serif;font-size:12px;">
<!--
            <select name="files" id="select_files" size="15" style="width:100%;">
-->
            </select>
        </div>
        <div class="fitem" style="padding-bottom:10px;">
           <input id="filename_re" class="easyui-textbox" data-options="validType:'filename_validation'" style="width:100%;">
        </div>
    </form>
</div>
<div id="dlg_buttons_re" style="display:none;">
   <a href="javascript:void(0)" id="submit_button_re"  class="easyui-linkbutton" iconCls="icon-ok" style="width:100px"><?php mea_toLocalC('open'); ?></a>
   <a href="javascript:void(0)" class="easyui-linkbutton" iconCls="icon-cancel" onclick="javascript:$('#dlg_re').dialog('close')" style="width:100px"><?php mea_toLocalC('cancel'); ?></a>
</div>

