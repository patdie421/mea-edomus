<?php
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');

session_start();
if(isset($_SESSION['language']))
{
   $LANG=$_SESSION['language'];
}
include_once('../lib/php/translation.php');
include_once('../lib/php/'.$LANG.'/translation.php');
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

<script type="text/javascript" src="controllers/indicateurs-ctrl.js"></script>

<script>
// voir http://jsfiddle.net/7j6xrnoa/5/ si nécessaire
function get_status(value) {
   return "<div id='status_" + value + "' class='pastille' style='float:left;background:gray'></div><b>" + value + "</b>";
}

jQuery(document).ready(function() {
   indicatorsTableController = new IndicatorsTableController("table_indicateurs");
   indicatorsTableController.linkToTranslationController(translationController); // pour la traduction des messages
   indicatorsTableController.linkToCredentialController(credentialController); // pour la gestion des habilitations

   var s=liveComController.getSocketio();
   if(s!=null) {
      var _rel_listener=indicatorsTableController.__rel_listener;
      var rel_listener=_rel_listener.bind(indicatorsTableController);
      s.on('rel', rel_listener);   

      var _mon_listener=indicatorsTableController.__mon_listener;
      var mon_listener=_mon_listener.bind(indicatorsTableController);
      s.on('mon', mon_listener);   
   }
   else {
      window.location="index.php";
   }

   indicatorsTableController.load();
});

</script>
<div style="width:100%;">
   <div style="width:700px;margin: 0 auto;">
      <table id="table_indicateurs" class="easyui-datagrid"  title="<?php mea_toLocalC('services status'); ?>" style="width:700px;" data-options="data:[],view:groupview,groupField:'Service',groupFormatter:function(value,rows){return get_status(value) + ' - ' + rows.length + ' ' + indicatorsTableController._toLocalC('indicator(s)');},onSelect:function(index,row){$('#table_indicateurs').datagrid('unselectRow', index);}">
         <thead>
            <tr>
               <th data-options="field:'Service',width:150,hidden:true"><?php mea_toLocalC('service'); ?></th>
               <th data-options="field:'Indicateur',width:200,align:'left'"><?php mea_toLocalC('indicator'); ?></th>
               <th data-options="field:'Description', width:400,align:'left'"><?php mea_toLocalC('description'); ?></th>
               <th data-options="field:'Valeur',width:80,align:'right'"><?php mea_toLocalC('value'); ?></th>
            </tr>
         </thead>
      </table>
   </div>
</div>

<div id="noindicators" class="display">
<?php mea_toLocalC('no indicator(s) provided'); echo " !!!"; ?>
</div>

