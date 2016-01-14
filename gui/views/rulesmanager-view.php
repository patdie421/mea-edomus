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

<script type="text/javascript" src="controllers/rulesmanagercontroller.js"></script>

<script>
// var current_file = false;

jQuery(document).ready(function() {

   // liaison view/controller
   ctrlr_rulesManager = new RulesManagerController("files_to_sel_rm", "files_sel_rm", "button_in_rm", "button_out_rm", "button_up_rm", "button_down_rm", "rulemgrmenu");
   ctrlr_rulesManager.linkToTranslationController(translationController); 
   ctrlr_rulesManager.linkToCredentialController(credentialController); 

   domenu_rm = ctrlr_rulesManager.domenu.bind(ctrlr_rulesManager);
});
</script>

<style>
.editable_ap {
}
</style>

<div id="rulemgrzone" style="width:auto;height:100%;">
   <div id="rulemgrmenu" style="height:28px; width:100%;padding-top:6px;border-bottom:1px solid #95B8E7;display:none">
      <a href="#" class="easyui-menubutton" data-options="menu:'#mm_rm'"><?php mea_toLocalC('Rules set'); ?></a>

<div id="mm_rm" style="width:150px; display:none">
      <div onclick="javascript:domenu_rm('new')"><?php mea_toLocalC('New'); ?></div>
      <div onclick="javascript:domenu_rm('load')"><?php mea_toLocalC('Load'); ?></div>
      <div onclick="javascript:domenu_rm('save')"><?php mea_toLocalC('Save'); ?></div>
      <div onclick="javascript:domenu_rm('saveas')"><?php mea_toLocalC('Save as'); ?></div>
      <div class="menu-sep"></div>
      <div onclick="javascript:domenu_rm('delete')"><?php mea_toLocalC('Delete'); ?></div>
   </div>
</div>
<div style="margin-top:30px">
<form id="fm_rm" method="post" novalidate>
    <div style="width:100%;text-align:center;">
        
        <div id="rm" class="easyui-panel" data-options="style:{margin:'0 auto'}" title="<?php mea_toLocalC_2d('choose rules to build'); ?>" style="width:700px;padding:10px;">
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
