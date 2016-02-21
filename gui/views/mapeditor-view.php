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

<script type="text/javascript" src="controllers/mapeditor-ctrl.js"></script>

<style type="text/css">
.drag {
}
.dp{
   opacity:0.5;
   filter:alpha(opacity=50);
}
.over{
   background:#FFFFCE;
}
.notover{
   background:#FFFFFF;
}
.widgetIcon{
   float:left;
   width:50px;
   height:50px;
   margin:2px;
   border:1px solid red;
   overflow:hidden;
}
</style>

<script>
jQuery(document).ready(function() {
   var list = [
      "../widgets/meawidget_value.js",
      "../widgets/meawidget_slider.js",
      "../widgets/meawidget_pastille.js",
      "../widgets/meawidget_button.js"
   ];

   ctrlr_mapEditor = new MapEditorController(
      "panel_me",
      "map_me",
      "properties_win_me",
      "actions_win_me",
      "new_win_me",
      "map_cm_me",
      "widget_cm_me");
   ctrlr_mapEditor.linkToTranslationController(translationController); 
   ctrlr_mapEditor.linkToCredentialController(credentialController); 

   var s=liveComController.getSocketio();
   if(s!=null) {
      var aut_listener=ctrlr_mapEditor.__aut_listener.bind(ctrlr_mapEditor);
      s.on('aut', aut_listener);
   }
   else {
      window.location="index.php";
   }

   var xplEditorUp     = ctrlr_mapEditor._xplEditorUp.bind(ctrlr_mapEditor);
   var xplEditorDown   = ctrlr_mapEditor._xplEditorDown.bind(ctrlr_mapEditor);
   var xplEditorOk     = ctrlr_mapEditor._xplEditorOk.bind(ctrlr_mapEditor);
   var xplEditorCancel = ctrlr_mapEditor._xplEditorCancel.bind(ctrlr_mapEditor);

   ctrlr_mapEditor.loadWidgets(list);

   ctrlr_mapEditor.start();

//   setTimeout(simu, 5000);

   $("#button_up_actions_win_me").on('click', xplEditorUp);
   $("#button_down_actions_win_me").on('click', xplEditorDown);
   $("#button_ok_ft_action_win_me").on('click', xplEditorOk);
   $("#button_cancel_ft_action_win_me").on('click', xplEditorCancel);

   $("#spectrum1").spectrum({
      color: "#f00",
      allowEmpty:true,
      showInput: true,
      showAlpha: true,
      chooseText: "set color",
      cancelText: "Cancel", 
      change: function(color) {
         ctrlr_mapEditor.map.css("background", '');
         ctrlr_mapEditor.map.css("background-size", '');
         ctrlr_mapEditor.map.css("background-color", color.toHexString());
         ctrlr_mapEditor.bgcolor = color.toHexString();
         ctrlr_mapEditor.bgimage = false;
      },
      showPalette:
         true,
         palette: [
            ["#000","#444","#666","#999","#ccc","#eee","#f3f3f3","#fff"],
            ["#f00","#f90","#ff0","#0f0","#0ff","#00f","#90f","#f0f"],
            ["#f4cccc","#fce5cd","#fff2cc","#d9ead3","#d0e0e3","#cfe2f3","#d9d2e9","#ead1dc"],
            ["#ea9999","#f9cb9c","#ffe599","#b6d7a8","#a2c4c9","#9fc5e8","#b4a7d6","#d5a6bd"],
            ["#e06666","#f6b26b","#ffd966","#93c47d","#76a5af","#6fa8dc","#8e7cc3","#c27ba0"],
            ["#c00","#e69138","#f1c232","#6aa84f","#45818e","#3d85c6","#674ea7","#a64d79"],
            ["#900","#b45f06","#bf9000","#38761d","#134f5c","#0b5394","#351c75","#741b47"],
            ["#600","#783f04","#7f6000","#274e13","#0c343d","#073763","#20124d","#4c1130"]
         ]
   });
/*
   $(document).mouseleave(function(e){console.log('out')});
   $(document).mouseenter(function(e){console.log('in')});
*/
});


</script>
<div class="easyui-panel" style="position:absolute;width:100%;height:100%;overflow:hidden" data-options="border:false">
   <div id="panel_me" class="scrolling" style="position:absolute;width:100%;height:100%;overflow:scroll;background:#EEEEEE">
   <div id="map_me" style="width:1920px;height:1080px;position:relative;overflow:auto;border:1px solid #555555;background:#FFFFFF">
      </div>
   <div id="widgets_container" style="display:none"></div>
</div>
</div>


<div id="map_cm_me" class="easyui-menu" style="width:180px;display:hidden;">
   <div id="map_cm_me_mode" name="toggle" onclick="javascript:ctrlr_mapEditor._context_menu('toggle')">to view mode</div>
   <div class="menu-sep"></div>
   <div>
      <span>grid</span>
      <div style="width:180px">
         <div onclick="javascript:ctrlr_mapEditor._context_menu('gridnone')">none</div>
         <div onclick="javascript:ctrlr_mapEditor._context_menu('grid5x5')">5 x 5</div>
         <div onclick="javascript:ctrlr_mapEditor._context_menu('grid10x10')">10 x 10</div>
         <div onclick="javascript:ctrlr_mapEditor._context_menu('grid20x20')">20 x 20</div>
         <div onclick="javascript:ctrlr_mapEditor._context_menu('grid50x50')">50 x 50</div>
      </div>
   </div>
   <div>
      <span>background</span>
      <div style="width:180px">
         <div id="spectrum1">color</div>
         <div onclick="javascript:ctrlr_mapEditor._context_menu('backgroundi')">image</div>
      </div>
   </div>

   <div class="menu-sep"></div>
   <div>
      <span>map</span>
      <div style="width:180px">
         <div onclick="javascript:ctrlr_mapEditor._context_menu('new', 'new_win_me')">new</div>
         <div onclick="javascript:ctrlr_mapEditor._context_menu('load')">load</div>
         <div onclick="javascript:ctrlr_mapEditor._context_menu('save')">save</div>
         <div onclick="javascript:ctrlr_mapEditor._context_menu('saveas')">save as</div>
         <div class="menu-sep"></div>
         <div onclick="javascript:ctrlr_mapEditor._context_menu('delete')">delete</div>
      </div>
   </div>
</div>


<div id="widget_cm_me" class="easyui-menu" style="width:120px;display:hidden">
   <div onclick="javascript:ctrlr_mapEditor._widget_menu('properties')">properties</div>
   <div class="menu-sep"></div>
   <div onclick="javascript:ctrlr_mapEditor._widget_menu('delete')">remove</div>
   <div class="menu-sep"></div>
   <div onclick="javascript:ctrlr_mapEditor._widget_menu('background')">background</div>
   <div onclick="javascript:ctrlr_mapEditor._widget_menu('backward')">backward</div>
   <div onclick="javascript:ctrlr_mapEditor._widget_menu('forward')">forward</div>
   <div onclick="javascript:ctrlr_mapEditor._widget_menu('forground')">forground</div>
</div>


<div id='widgetsPanel_win_me' class='easyui-window win_me' title='Widgets' style='width:210px;height:500px'>
   <div id='panel_widgetsPanel_win_me' style='width:100%;height:100%;position:relative;overflow:auto'>
      <div id='accordion_widgetsPanel_win_me' class='easyui-accordion' style='width:100%;height:100%;'>
      </div>
   </div>
</div>


<div id="new_win_me" class="easyui-window win_me" style="position:relative;width:300px;height:180px;overflow:hidden" data-options="title:'map size',modal:true,closed:true,footer:'#ft_new_win_me'">
   <table cellpadding="2" style="width:100%">
      <col width="45%">
      <col width="10%">
      <col width="45%">

      <tr height="5">
      </tr>

      <tr>
         <td align="center" colspan="3">
            <select id="select_new_win_me" class="easyui-combobox" style="width:160px;" data-options="editable:false">
               <option value='custom'>custom</option>
               <option value='{"width":"1024", "height":"768"}'>1024 x 768</option>
               <option value='{"width":"1280", "height":"720"}'>1280 x 720 (720 p/i)</option>
               <option value='{"width":"1920", "height":"1080"}'>1920 x 1080 (1080 p/i)</option>
            </select>
         </td> 
      </tr>

      <tr height="5">
      </tr>

      <tr>
         <td align="center">width</td>
         <td></td>
         <td align="center">height</td>
      </tr>

      <tr>
         <td align="center"><input id="input_width_new_win_me" class="easyui-textbox" style="width:100%;height:24px"></td>
         <td align="center">x</td>
         <td align="center"><input id="input_height_new_win_me" class="easyui-textbox" style="width:100%;height:24px"></td>
      </tr>

   </table>
</div>
<div id="ft_new_win_me" style="text-align:right;padding:5px;style:hidden">
      <a id="button_ok_ft_new_win_me"      href="javascript:void(0)" class="easyui-linkbutton" style="width=50px;" data-options="iconCls:'icon-ok'"><?php mea_toLocalC('ok'); ?></a>
      <a id="button_cancel_ft_new_win_me", href="javascript:void(0)" class="easyui-linkbutton" style="width=50px;" data-options="iconCls:'icon-cancel'"><?php mea_toLocalC('cancel'); ?></a>
</div>


<div id="actions_win_me" class="easyui-window win_me" style="position:relative;width:500px;height:360px;overflow:hidden" data-options="title:'xPL send parameters',modal:true,closed:true,footer:'#ft_action_win_me'">
   <table cellpadding="5" style="width:100%">
      <tr>
         <td align="center">Name</td>
         <td></td>
         <td align="center">Value</td>
      </tr>
      <tr>
         <td align="center">
            <input id="actions_win_me_names" name="names" class="easyui-combobox" style="width:200px;" data-options="valueField: 'label', textField: 'value'" />
         </td>
         <td>=</td>
         <td align="center">
            <input id="actions_win_me_values" name="values" class="easyui-combobox" style="width:200px;" data-options="valueField: 'label', textField: 'value'" />
         </td>
      </tr>
      <tr>
         <td align="center" colspan="3">
            <a id="button_up_actions_win_me"    href="javascript:void(0)" class="easyui-linkbutton" style="width=50px;" data-options="iconCls:'icon-meauparrow'"></a>
            <a id="button_down_actions_win_me", href="javascript:void(0)" class="easyui-linkbutton" style="width=50px;" data-options="iconCls:'icon-meadownarrow'"></a>
         </td>
      </td>
      <tr>
         <td align="center" colspan="3">
<!--
            <select id="actions_win_me_namesvalues" name="namesvalues" size="4" style="text-align:center;width:80%;font-family:verdana,helvetica,arial,sans-serif;font-size:12px;">
            </select>
-->
            <div id="actions_win_me_namesvalues2"  name="namesvalues2"></div>
         </td>
      </tr>
   </table>
   <input id="actions_win_me_widget" name="widgetid_me" type="hidden">
   <input id="actions_win_me_action" name="action_me" type="hidden">
</div>
<div id="ft_action_win_me" style="text-align:right;padding:5px;display:hidden">
   <a id="button_ok_ft_action_win_me"      href="javascript:void(0)" class="easyui-linkbutton" style="width=50px;" data-options="iconCls:'icon-ok'"><?php mea_toLocalC('ok');?></a>
   <a id="button_cancel_ft_action_win_me", href="javascript:void(0)" class="easyui-linkbutton" style="width=50px;" data-options="iconCls:'icon-cancel'"><?php mea_toLocalC('cancel');?></a>
</div>
