<?php
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');

session_start();
if(isset($_SESSION['language']))
{
   $LANG=$_SESSION['language'];
}
include_once('../lib/php/translation.php');
include_once('../lib/php/$LANG/translation.php');
mea_loadTranslationData($LANG,'../');

$isadmin = check_admin();
if($isadmin !=0 && $isadmin != 98) : ?>
<script>
   window.location = "login.php";
</script>
<?php
   exit(1);
endif;
?>

<script type="text/javascript" src="controllers/application-ctrl.js"></script>
<script type="text/javascript" src="lib/js/fsselector.js"></script>

<script>
<?php
if($isadmin==0) : ?>
function fsPluginPath()
{
   fileselector.directory(translationController.toLocalC('plugin path'), 500, 400, 'PLUGINPATH');
}

function fsBufferPath()
{
   fileselector.directory(translationController.toLocalC('buffer path'), 500, 400, 'BUFFERDB');
}
<?php
endif; ?>

<?php
if($isadmin!=0) : ?>
function viewonly_ap()
{
  $(".editable_ap").textbox({editable: false, disabled:true});
}
<?php
endif; ?>

jQuery(document).ready(function(){
   applicationPrefsController = new ApplicationPrefsController();
   fileselector = new FSSelector("models/get_dir.php");
   
   applicationPrefsController.linkToTranslationController(translationController);

<?php
   if($isadmin!=0) : ?>
      viewonly_ap();
<?php
   endif; ?>
   applicationPrefsController.load();
});
</script>

<style>
.editable_ap {
}
</style>


<form id="fm_ap" method="post" novalidate>
    <div style="width:100%;text-align:center;">
        
        <div style="margin:0 auto; width:700px; padding-bottom:20px; text-align:right;">
           <a href="javascript:void(0)" class="easyui-linkbutton" onclick="javascript:applicationPrefsController.load()" data-options="iconCls:'icon-reload'"><?php mea_toLocalC('reload'); ?></a>
<?php
if($isadmin==0) : ?>
           <a href="javascript:void(0)" class="easyui-linkbutton" onclick="javascript:applicationPrefsController.save()" data-options="iconCls:'icon-save'"><?php mea_toLocalC('save'); ?></a>
<?php
endif; ?>
        </div>
        
        <div id="ap1" class="easyui-panel" data-options="style:{margin:'0 auto'}" title="<?php mea_toLocalC('xPL Address'); ?>" style="width:700px;padding:10px;">
            <table width="500px" align="center" style="padding-bottom: 12px;">
                <col width="100px">
                <col width="30px">
                <col width="100px">
                <col width="30px">
                <col width="100px">
                <tr>
                    <td align="center">
                        <label for="VENDORID" id="vendorid"><?php mea_toLocalC('vendor ID');?></label>
                    </td>
                    <td></td>
                    <td align="center">
                        <label for="DEVICEID" id="deviceid"><?php mea_toLocalC('device ID');?></label>
                    </td>
                    <td></td>
                    <td align="center">
                        <label for="INSTANCEID" id="instanceid"><?php mea_toLocalC('Instance ID');?></label>
                    </td>
                </tr>
                <tr>
                    <td align="center">
                        <input class="easyui-textbox editable_ap" name="ENDORID" id="VENDORID" data-options="required:true, validType:'name_validation', missingMessage:translationController.toLocalC('vendor ID name is mandatory')" style="height:25px; text-align: center;"/>
                    </td>
                    <td align="center">
                        <div style="padding:0.5em;">-</div>
                    </td>
                    <td align="center">
                        <input class="easyui-textbox editable_ap" name="DEVICEID" id="DEVICEID" style="height:25px; text-align: center;" data-options="required:true,validType:'name_validation',missingMessage:translationController.toLocalC('device ID name is mandatory')" />
                    </td>
                    <td align="center">
                        <div style="padding: 0.5em;">.</div>
                    </td>
                    <td align="center">
                        <input class="easyui-textbox editable_ap" name="INSTANCEID" id="INSTANCEID" style="height:25px; text-align: center;" data-options="required:true,validType:'device_name_validation',missingMessage:translationController.toLocalC('instance ID name is mandatory')" />
                    </td>
                </tr>
            </table>
        </div>
        <p></p>
        <div id="ap2" class="easyui-panel" data-options="style:{margin:'0 auto'}" title="<?php mea_toLocalC('plugin path'); ?>" style="width:700px;padding:10px;">
            <table width="600px" align="center" style="padding-bottom: 12px;">
                <col width="30%">
                <col width="70%">
                <tr>
                    <td align="right">
                        <label for="PLUGINPATH" id="libplugins"><?php mea_toLocalC_2d('plugins library');?></label>
                    </td>
                    <td>
<?php
if($isadmin==0) : ?>
                        <input class="easyui-textbox editable_ap" style="height:25px; width:100%; margin-bottom:0px;" name="PLUGINPATH" id="PLUGINPATH" data-options="buttonText:translationController.toLocalC('select'),prompt:translationController.toLocalC('path...'),onClickButton:fsPluginPath"/>
<?php
else : ?>
                        <input class="easyui-textbox editable_ap" style="height:25px; width:100%; margin-bottom:0px;" name="PLUGINPATH" id="PLUGINPATH"/>
<?php
endif; ?>
                    </td>

                </tr>
            </table>
        </div>
        <p></p>
        <div id="ap3" class="easyui-panel" data-options="style:{margin:'0 auto'}" title="<?php mea_toLocalC('MySQL history database connection');?>" style="width:700px;padding:10px;">
           <table width="600px" align="center" >
              <col width="30%">
              <col width="30%">
              <col width="10%">
              <col width="30%">
              <tr>
                 <td align="right">
                    <label for="DBSERVER" id="mysqlserver"><?php mea_toLocalC_2d('Server name/address');?></label>
                 </td>
                 <td>
                    <input name="DBSERVER" id="DBSERVER" style="height:25px; width:100%;" class="easyui-textbox editable_ap" />
                 </td>
                 <td align="right">
                    <label for="DBPORT" id="port"><?php mea_toLocalC_2d('port');?></label>
                 </td>
                 <td>
                   <input type="text" name="DBPORT" id="DBPORT" style="height:25px; width:50%;" class="easyui-textbox editable_ap" />
                 </td>
              </tr>
              <tr>
                 <td align="right">
                    <label for="DATABASE" id="base"><?php mea_toLocalC_2d('database name');?></label>
                 </td>
                 <td>
                    <input type="text" name="DATABASE" id="DATABASE" style="height:25px; width:100%;" class="easyui-textbox editable_ap" />
                 </td>
              </tr>
              <tr>
                 <td align="right">
                    <label for="USER" id="user"><?php mea_toLocalC_2d('user name');?></label>
                 </td>
                 <td>
                    <input type="text" name="USER" id="USER" style="height:25px; width:100%;" class="easyui-textbox editable_ap" />
                 </td>
              </tr>
              <tr>
                 <td align="right">
                    <label for="PASSWORD" id="passwdx"><?php mea_toLocalC_2d('user password');?></label>
                 </td>
                 <td>
                    <input type="password" name="PASSWORD" align="right" id="PASSWORD" style="height:25px; width:100%;" class="easyui-textbox editable_ap" />
                 </td>
              </tr>
              <tr>
                  <td>
                  </td>
                  <td>
<?php
if($isadmin==0) : ?>
                  <div style="text-align: center; padding:6px">
                    <a href="javascript:void(0)" class="easyui-linkbutton easyui-tooltip" onclick="javascript:applicationPrefsController.checkMysql($('#DBSERVER').val(), $('#DBPORT').val(), $('#DATABASE').val(), $('#USER').val(), $('#PASSWORD').val())" style="height:25px; " title="<?php mea_toLocalC('Validate end-to-end database connection.'); ?>"><?php mea_toLocalC('test connection');?></a>
                  </div>
<?php
endif;
?>
                  </td>
              </tr>
           </table>
        </div>
        <p></p>
        <div id="ap4" class="easyui-panel" data-options="style:{margin:'0 auto'}" title="<?php mea_toLocalC('continuity buffer (used when Mysql history database is unavailable)');?>" style="width:700px;padding:10px;">
            <table width="600px" align="center" style="padding-bottom: 12px;">
               <col width="30%">
               <col width="60%">
               <col width="10%">
               <tr>
                  <td align="right">
                     <label for="BUFFERDB" id="sqlitebuffer"><?php mea_toLocalC_2d('SQLite buffer db'); ?></label>
                  </td>
                  <td>
<?php
if($isadmin==0) : ?>
                     <input name="BUFFERDB" id="BUFFERDB" style="height:25px; width:100%; " class="easyui-textbox editable_ap" data-options="buttonText:'Select',prompt:'path...',onClickButton:fsBufferPath"/>
<?php
else : ?>
                     <input name="BUFFERDB" id="BUFFERDB" style="height:25px; width:100%; " class="easyui-textbox editable_ap"/>
<?php
endif; ?>
                  </td>
                  <td valign="bottom" align="center">
<?php
if($isadmin==0) : ?>
                     <a href="javascript:void(0)" class="easyui-linkbutton easyui-tooltip" onclick="javascript:applicationPrefsController.ceate_querydb($('#BUFFERDB').val)" style="width:100%; height:25px" title="<?php mea_toLocalC('Create the database stucture.');?>"><?php mea_toLocalC('init');?></a>
<?php
endif; ?>
                  </td>
               </tr>
            </table>
       </div>
       <div style="margin:0 auto; width:700px; padding-top:20px; text-align:right;">
           <a href="javascript:void(0)" class="easyui-linkbutton" onclick="javascript:applicationPrefsController.load()" data-options="iconCls:'icon-reload'"><?php mea_toLocalC('reload'); ?></a>
<?php
if($isadmin==0) : ?>
           <a href="javascript:void(0)" class="easyui-linkbutton" onclick="javascript:applicationPrefsController.save()" data-options="iconCls:'icon-save'"><?php mea_toLocalC('save'); ?></a>
<?php
endif; ?>
       </div>
   </div>
</form>
