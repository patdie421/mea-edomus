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


<script type="text/javascript" src="controllers/mapsseteditorcontroller.js"></script>

<script>
function valueCheckExist(v, data)
{
   var found = false;
   $.each(data, function(i, value) {
      if(value.f1 == v)
      {
         found=true;
         return false;
      }
   });

   return found;
}


function valueFromText(t, data)
{
   var found = false;
   $.each(data, function(i, value) {
      if(value.f2 == t)
      {
         found=value.f1;
         return false;
      }
   });
   return found;
}


function updateCombo(id, data)
{
   var cbx = $("#"+id);
   var s = cbx.combobox('getValue');

   cbx.combobox({ data: data });

   if(valueCheckExist(s, data))
      cbx.combobox('setValue', s);
}


function updateSelectedCallback(data)
{
   updateCombo("defaultmap", data);
   updateCombo("sc1", data);
   updateCombo("sc2", data);
   updateCombo("sc3", data);
   updateCombo("sc4", data);
}


function setComboboxValue(obj, value, list)
{
   var v="";
   if(list)
   {
      v=valueFromText(value, list);
      if(!v)
         v="";
   }
   obj.combobox('setValue', v);
}


function getComboboxValue(obj)
{
   value = obj.combobox('getText');

   return value;
}


jQuery(document).ready(function() {
   // liaison view/controller
   $("#defaultmap").combobox({
      valueField:'f1',
      textField:'f2',
      data: []
   });

   $("#sc1").combobox({
      valueField:'f1',
      textField:'f2',
      data: []
   });

   $("#sc2").combobox({
      valueField:'f1',
      textField:'f2',
      data: []
   });

   $("#sc3").combobox({
      valueField:'f1',
      textField:'f2',
      data: []
   });

   $("#sc4").combobox({
      valueField:'f1',
      textField:'f2',
      data: []
   });

   ctrlr_mapsseteditor = new MapsSetEditorController(
      "menueditor",
      "menueditormenu",
      "panel_mu",
      "maps_to_select_mu",
      "maps_selected_mu",
      "button_in_mu",
      "button_out_mu",
      "button_up_mu",
      "button_down_mu",
      "button_save_mu",
      updateSelectedCallback,
      {
         defaultmap: {
            obj: $("#defaultmap"),
            set: setComboboxValue,
            get: getComboboxValue
         },
         shortcut1: {
            obj: $("#sc1"),
            set: setComboboxValue,
            get: getComboboxValue
         },
         shortcut2: {
            obj: $("#sc2"),
            set: setComboboxValue,
            get: getComboboxValue
         },
         shortcut3: {
            obj: $("#sc3"),
            set: setComboboxValue,
            get: getComboboxValue
         },
         shortcut4: {
            obj: $("#sc4"),
            set: setComboboxValue,
            get: getComboboxValue
         }
      }
   );

   ctrlr_mapsseteditor.linkToTranslationController(translationController); 
   ctrlr_mapsseteditor.linkToCredentialController(credentialController); 

   domenu_mu = ctrlr_mapsseteditor.domenu.bind(ctrlr_mapsseteditor);

   ctrlr_mapsseteditor.start();
});

</script>

<div id="menueditor" style="width:auto;height:100%;">
   <div id="menueditormenu" style="height:28px; width:100%;padding-top:6px;border-bottom:1px solid #95B8E7;display:none">
      <a href="#" class="easyui-menubutton" data-options="menu:'#mm1_mu'"><?php mea_toLocalC('maps set editor'); ?></a>

      <div id="mm1_mu" style="width:150px; display:none">
         <div onclick="javascript:domenu_mu('new')"><?php mea_toLocalC('New'); ?></div>
         <div onclick="javascript:domenu_mu('load')"><?php mea_toLocalC('Load'); ?></div>
         <div onclick="javascript:domenu_mu('save')"><?php mea_toLocalC('Save'); ?></div>
         <div onclick="javascript:domenu_mu('saveas')"><?php mea_toLocalC('Save as'); ?></div>
         <div class="menu-sep"></div>
         <div onclick="javascript:domenu_mu('delete')"><?php mea_toLocalC('Delete'); ?></div>
      </div>
   </div>

   <div id="panel_mu" class="easyui-panel" data-options="border:false">
      <form id="fm_mu" method="post" novalidate>
         <div style="width:100%;text-align:center;padding-top:20px;padding-bottom:20px">
            <div id="mu1" class="easyui-panel" data-options="style:{margin:'0 auto'}" title="<?php mea_toLocalC_2d('choose maps to use'); ?>" style="width:700px;padding:10px;">
               <table width="600px" align="center" style="padding-bottom: 12px;">
                  <col width="40%">
                  <col width="10%">
                  <col width="40%">
                  <col width="10%">

                  <tr>
                     <td align="center"><B><?php mea_toLocalC('available'); ?></B></td>
                     <td></td>
                     <td align="center"><B><?php mea_toLocalC('selected'); ?></B></td>
                     <td></td>
                  </tr>

                  <tr>
                     <td align="center">
                        <div id="maps_to_select_mu" style="width:100%;height:200px;">
                        </div>
                     </td>
                     <td align="center">
                        <table>
                           <tr><td>
                              <a id="button_in_mu" href="javascript:void(0)" data-options="iconCls:'icon-mearightarrow'" class="easyui-linkbutton" style="width:30px;height:30px"></a>
                           </td></tr>
                           <tr><td>
                              <a id="button_out_mu" href="javascript:void(0)" data-options="iconCls:'icon-mealeftarrow'" class="easyui-linkbutton" style="width:30px;height:30px"></a>
                           </td></tr>
                        </table>
                     </td>

                     <td align="center">
                        <div id="maps_selected_mu" style="width:100%;height:200px;">
                     </td>
                     <td align="center">
                        <table>
                           <tr><td>
                              <a id="button_up_mu" href="javascript:void(0)" data-options="iconCls:'icon-meauparrow'" class="easyui-linkbutton" style="width:30px;height:30px"></a>
                           </td></tr>
                           <tr><td>
                              <a id="button_down_mu" href="javascript:void(0)" data-options="iconCls:'icon-meadownarrow'" class="easyui-linkbutton" style="width:30px;height:30px"></a>
                           </td></tr>
                        </table>
                     </td>
                  </tr>
               </table>

            </div>
            <p></p>
            <div id="mu1" class="easyui-panel" data-options="style:{margin:'0 auto'}" title="<?php mea_toLocalC_2d('default map and menu sortcuts'); ?>" style="width:700px;padding:10px;">
               <div>
                     <table width="600px" align="center" style="padding-bottom: 12px;">
                        <col width="40%">
                        <col width="60%">
                        <tr>
                           <td align="right">
                              <label><?php mea_toLocalC_2d('Default map'); ?></label>
                           </td>
                           <td>
                              <input id=defaultmap style='width:150px'>
                           </td>
                        </tr>
                        <tr>
                           <td>
                           <p></p>
                           </td>
                        </tr>
                        <tr>
                           <td align="right">
                              <label><?php mea_toLocalC('short cut '); mea_toLocalC_2d('1') ?></label>
                           </td>
                           <td>
                              <input id=sc1 style='width:150px'>
                           </td>
                        </tr>
                        <tr>
                           <td align="right">
                              <label><?php mea_toLocalC('short cut '); mea_toLocalC_2d('2') ?></label>
                           </td>
                           <td>
                              <input id=sc2 style='width:150px'>
                           </td>
                        </tr>
                        <tr>
                           <td align="right">
                              <label><?php mea_toLocalC('short cut '); mea_toLocalC_2d('3') ?></label>
                           </td>
                           <td>
                              <input id=sc3 style='width:150px'>
                           </td>
                        </tr>
                        <tr>
                           <td align="right">
                              <label><?php mea_toLocalC('short cut '); mea_toLocalC_2d('4') ?></label>
                           </td>
                           <td>
                              <input id=sc4 style='width:150px'>
                           </td>
                        </tr>
                     </table>
               </div>
            </div>
         </div>

         <p></p>

         <div style="margin:0 auto; text-align:center;">
            <a id="button_save_mu" href="javascript:void(0)" class="easyui-linkbutton" style="width=50px;" data-options="iconCls:'icon-save'"><?php mea_toLocalC('save'); ?></a>
         </div>
      </form>
   </div>
</div>
