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

<style type="text/css">
   .drag {
   }

   .dp{
      opacity:0.5;
      filter:alpha(opacity=50);
   }
   .over{
      background:#FBEC88;
   }
</style>


<script>
$.fn.datagrid.defaults.editors.empty = {
	 init: function(container, options){
		 return $('<div style="padding:0 4px"></div>').appendTo(container);
	 },
	 getValue: function(target){
		 return $(target).html();
	 },
	 setValue: function(target, value){
		 $(target).html(value);
	 }
};


/*
function context_menu(action)
{
   switch(action)
   {
      case 'list':
         $('div[id^="widget_"]').each(function(){
            var id = $(this).attr('id')
            var t  = $(this).attr('mea_widget')
            var x  = parseInt($(this).css("top"));
            var y  = parseInt($(this).css("left"));
            console.log(id+": x="+x+" y="+y+" ("+t+")");
         });
         break;

      case 'disable':
         $('div[id^="widget_"]').each(function(){
            $(this).draggable('disable'); 
         });
         break;
      case 'enable':
         $('div[id^="widget_"]').each(function(){
            $(this).draggable('enable'); 
         });
   }
}
*/

function MapEditorController(container, map, toolsPanel, propertiesPanel, mapContextMenu, widgetContextMenu)
{
   MapEditorController.superConstructor.call(this);

   this.container =         $('#'+container);
   this.map =               $('#'+map);
   this.toolsPanel =        $('#'+toolsPanel);
   this.propertiesPanel =   $('#'+propertiesPanel);
   this.mapContextMenu =    $('#'+mapContextMenu);
   this.widgetContextMenu = $('#'+widgetContextMenu);

   this.current_zindex=this.max_zIndex(map);

   this.mea_mouse_x = -1;
   this.mea_mouse_y = -1;

   this.objid = 10;
   this.timeout = false;

   var _this = this;

   _this.getmousepos_handler = _this._getmousepos_handler.bind(_this);
   _this.scroll = _this._scroll.bind(_this);
   _this.constrain = _this._constrain.bind(_this);
   _this.open_context_menu = _this._open_context_menu.bind(_this);
   _this.context_menu = _this._context_menu.bind(_this);
   _this._open_widget_menu = _this.__open_widget_menu.bind(_this);
   _this.widget_menu = _this._widget_menu.bind(_this);
   _this.widgetContextMenu.menu({onHide: function() { _this.widgetContextMenu.attr('mea_eid', false); } });
   _this.map.bind('contextmenu', _this.open_context_menu);

   var options = {
      onBeforeDrag: function(e)
      {
         _this._updateProperties($(this).attr('id'));

         zindex = $(this).css("z-index");
         if(zindex != _this.current_zindex)
         $(this).css("z-index", ++_this.current_zindex);
         $('body').append($(this));
         $(this).hide();
      },

      onStartDrag: function(e)
      {
         $(this).draggable('proxy').addClass('dp');
         $('body').mousemove(_this.getmousepos_handler);
      },

      onStopDrag: function(e)
      {
         var d = e.data;

         $('body').mousemove(_this.getmousepos_handler);

         offset = _this.map.offset();

         l = _this.repair(d.left - offset.left);
         t = _this.repair(d.top - offset.top);

         var p = $(this).clone();
         var source = $(this).prop('mea-widgetdata');
 
         p.css({top: t, left: l}).draggable(options);
         p.css("z-index", ++_this.current_zindex);
         p.bind('contextmenu', _this.open_widget_menu);

         p.prop('_me', _this);
         p.prop('mea-widgetdata',source);

         _this.map.append(p);

         p.show();

         $(this).remove();

         _this._updateProperties(p.attr('id'));
      },

      proxy: function(source) {
         var type=$(source).attr("mea_widget");
         var p = $("#"+type+"_model").clone().attr('id', type+'_drag');
         $(p).css("z-index", 999999);
         p.appendTo('body');
         return p;
      },

      onDrag: function(e) {
        var d = e.data;
        var id=$(e.data.target).attr('mea_widget');

        _this.constrain(e, $("#"+id+"_drag"), $("#"+id+"_model"));
      }
   };

   _this.propertiesPanel.append("<DIV id='div_properties' style='width:100%;height:auto'><TABLE id='tbl_properties' style='width:100%'></TABLE><DIV>");
   _this.propertiesTbl = $("#tbl_properties");
   _this.propertiesTbl.propertygrid({
      showGroup:true,
      border:false,
      scrollbarSize:0,
      rowStyler:function(index, row) {
         if(row.editor == 'empty')
         {
            return 'background-color:pink;color:blue;font-weight:bold;';
         }
      },
      onSelect:function(index, row)
      {
         var __this = this;
         $(__this).propertygrid('beginEdit', index);
         var ed = $(__this).propertygrid('getEditor', { index:index, field:'value'});
         $(ed.target).focus().val($(ed.target).val()); // pour positionner le curseur à la fin du champ
      },
      onBeginEdit:function(row) {
         var i = row;
         var __this = this;
         var ed = $(__this).propertygrid('getEditor', { index:i, field:'value'});
         var cell = $(ed.target);
         cell.bind('keyup', function(e) { c = e.keyCode || e.which(); if(c == 13) { $(__this).propertygrid('endEdit', i); } });
      },
      onEndEdit:function(row) {
         var i = row;
         var __this = this;

         var ed = $(__this).propertygrid('getEditor', { index:i, field:'value'});
         var cell = $(ed.target);

         cell.unbind('keyup');
         cell.blur();
         $(__this).propertygrid('unselectAll');
      }
   });

   _this.toolsPanel.window({
      top:-100,
      left:-100,
      onMove: function(left,top){
         if (left<0) {
            $(this).window('move',{
               left:0
            });
         }
         if (top<0){
            $(this).window('move',{
               top:0
            });
         }
      }
   });

   _this.map.droppable({
      accept:'.drag',
      onDragEnter:function(e,source){
         $(source).draggable('options').cursor='auto';
         $(source).draggable('proxy').css('border','2px solid red');
         $(this).addClass('over');
      },

      onDragLeave:function(e,source){
         $(source).draggable('options').cursor='not-allowed';
         $(source).draggable('proxy').css('border','2px solid #ccc');
         $(this).removeClass('over');
      },

      onDrop:function(e,source) {
         _this.objid++;
         var type=$(source).attr("id");

         var offset = _this.map.offset();
         var newid = "widget_"+type+"_"+_this.objid;

         var p = $("#"+type+"_model").clone().attr('id', newid);

         $(this).append(p);
         var t=$("#"+type+"_drag").offset().top;
         var l=$("#"+type+"_drag").offset().left;

         $("#"+newid).css({top: t - offset.top, left: l - offset.left}).draggable(options);
         $("#"+newid).bind('contextmenu', _this.open_widget_menu);

         $("#"+newid).prop('mea-widgetdata',  [
            {"name":"id",   "value":newid, "group":"Indentification", "editor":"empty", "mea-data":{'readOnly': true} },
            {"name":"type", "value":type,  "group":"Indentification", "editor":"empty",  "mea-data":{}},
            {"name":"x",    "value":t - offset.top,  "group":"Position", "editor":"empty",  "mea-data":{}},
            {"name":"y",    "value":l - offset.left,  "group":"Position", "editor":"empty",  "mea-data":{}},
            {"name":"value1", "value":"",  "group":"links", "editor":"text",  "mea-data":{}},
            {"name":"value2", "value":"",  "group":"links", "editor":"text",  "mea-data":{}},
            {"name":"value3", "value":"",  "group":"links", "editor":"text",  "mea-data":{}},
         ]);
         _this._updateProperties(newid);

         $(this).removeClass('over');
      }
   });
}

extendClass(MapEditorController, CommonController);


MapEditorController.prototype.start = function()
{
}


MapEditorController.prototype._context_menu = function(action)
{
   var _this = this;

   switch(action)
   {
      case 'list':
         $('div[id^="widget_"]').each(function(){
            var id = $(this).attr('id')
            var t  = $(this).attr('mea_widget')
            var x  = parseInt($(this).css("top"));
            var y  = parseInt($(this).css("left"));
            console.log(id+": x="+x+" y="+y+" ("+t+")");
            console.log(JSON.stringify($(this).prop('mea-widgetdata')));
         });
         break;

      case 'disable':
         $('div[id^="widget_"]').each(function(){
            $(this).draggable('disable');
         });
         break;
      case 'enable':
         $('div[id^="widget_"]').each(function(){
            $(this).draggable('enable');
         });
   }
}


MapEditorController.prototype._updateProperties = function(id)
{
   var _this = this;

   var y  = parseInt($("#"+id).css("top"));
   var x  = parseInt($("#"+id).css("left"));

   var source = $("#"+id).prop('mea-widgetdata');
   source[2].value = x;
   source[3].value = y;
   _this.propertiesTbl.propertygrid({data: source});

   setTimeout(function()
   {
      _this.propertiesPanel.window('resize', {height: $('#div_properties').height()+40 });
   }
   , 1);
}


MapEditorController.prototype._widget_menu = function(action)
{
   var _this = this;

   switch(action)
   {
      case 'delete':
         var id = _this.widgetContextMenu.attr('mea_eid');
         _this.propertiesTbl.propertygrid({data:[]});
         $("#"+id).empty();
         $("#"+id).remove();
         return false;

      case 'properties':
         _this.propertiesPanel.window('open');
         var id = _this.widgetContextMenu.attr('mea_eid');
         _this._updateProperties(id);
         return false;
   }
}


MapEditorController.prototype.max_zIndex = function(div)
{
   var _this = this;
   var zi = 0;
   var max = 0;

   console.log(div);
   $('#'+div+' >').each(function(){
      zi=$(this).css('zIndex');
      if(zi > max)
         max = zi;
   });

   return max;
}


MapEditorController.prototype.repair = function(v)
{
   var r = parseInt(v/20)*20;
   if (Math.abs(v % 20) > 10) {
      r += v > 0 ? 20 : -20;
   }
   return r;
}


MapEditorController.prototype._scroll = function()
{
   var _this = this;

   offset = _this.container.offset();
   x1=offset.left;
   y1=offset.top;
   x2=x1+_this.container.width();
   y2=y1+_this.container.height();

   if(_this.mea_mouse_y < y1)
      _this.container.scrollTop(_this.container.scrollTop() - 25);
   if(_this.mea_mouse_y > y2)
      _this.container.scrollTop(_this.container.scrollTop() + 25);
   if(_this.mea_mouse_x < x1)
      _this.container.scrollLeft(_this.container.scrollLeft() - 25);
   if(_this.mea_mouse_x > x2)
      _this.container.scrollLeft(_this.container.scrollLeft() + 25);

   _this.timeout=setTimeout(_this.scroll, 50);
}


MapEditorController.prototype._constrain = function(e, drag, model)
{
   var _this = this;

   var d = e.data;
   var offset = _this.container.offset();

   var l_min = offset.left;
   var l_max = _this.container.outerWidth()+offset.left;
   var t_min = offset.top;
   var t_max = _this.container.outerHeight()+offset.top;

   clearTimeout(_this.timeout);

   if (d.left < l_min)
   {
      _this.container.scrollLeft(_this.container.scrollLeft() - 25);
      _this.timeout=setTimeout(_this.scroll, 50);
      d.left = offset.left;
   }

   if (d.top < t_min)
   {
      _this.container.scrollTop(_this.container.scrollTop() - 25);
      _this.timeout=setTimeout(_this.scroll, 50);
      d.top = offset.top;
   }

   if (d.left + model.outerWidth() > l_max)
   {
      _this.container.scrollLeft(_this.container.scrollLeft() + 25);
      _this.timeout=setTimeout(_this.scroll, 50);
      d.left = l_max - model.outerWidth();
   }

   if (d.top + model.outerHeight() > t_max)
   {
      _this.container.scrollTop(_this.container.scrollTop() + 25);
      _this.timeout=setTimeout(_this.scroll, 150);
      d.top = t_max - model.outerHeight();
   }
}


MapEditorController.prototype._getmousepos_handler = function( event ) {
   var _this = this;

   _this.mea_mouse_x = event.pageX;
   _this.mea_mouse_y = event.pageY;
}


MapEditorController.prototype.__open_widget_menu = function(id, x, y)
{
   var _this = this;

   _this.widgetContextMenu.attr('mea_eid', id);
   _this.widgetContextMenu.menu('show', {left: x, top: y });
}


MapEditorController.prototype.open_widget_menu = function(e)
{
   var _this = this;
   e.preventDefault();

   _me = $(_this).prop("_me");

   _me._open_widget_menu($(_this).attr('id'), e.pageX,  e.pageY);

   return false;
}


MapEditorController.prototype._open_context_menu = function(e)
{
   var _this = this;

   e.preventDefault();

   _this.mapContextMenu.menu('show', {left: e.pageX, top: e.pageY });

   return false;
}


MapEditorController.prototype.addWidget = function(type)
{
   var _this = this;

   $("#"+type).draggable({
      proxy: function(source) {
         var p = $("#"+type+"_model").clone().attr('id', type+'_drag');
         $(p).css("z-index", 999999);
         p.appendTo('body');
         return p;
      },
      revert:true,
      cursor:'auto',

      onDrag: function(e) {
         var id=$(e.data.target).attr('id');
         _this.constrain(e, $("#"+id+"_drag"), $("#"+id+"_model"));
      },

      onStopDrag: function(e) {
         var __this = this; 
         $(__this).draggable('options').cursor='auto';
         $('body').unbind('mousemove',  _this.getmousepos_handler);
      },

      onStartDrag:function(){
         var __this = this;
         $(__this).draggable('options').cursor='not-allowed';
         $(__this).draggable('proxy').addClass('dp');
         $('body').mousemove(_this.getmousepos_handler);
      }
   });
}


jQuery(document).ready(function() {
  
   ctrlr_mapEditor = new MapEditorController(
      "me_panel",
      "testzone",
      "win",
      "wn_properties",
      "cm_context",
      "cm_widget");
   ctrlr_mapEditor.linkToTranslationController(translationController); 
   ctrlr_mapEditor.linkToCredentialController(credentialController); 

   ctrlr_mapEditor.start();

   ctrlr_mapEditor.addWidget("switch");
   ctrlr_mapEditor.addWidget("jauge");
   ctrlr_mapEditor.addWidget("toggle");
/*
   $(document).mouseleave(function(e){console.log('ou')});
   $(document).mouseenter(function(e){console.log('in')});
*/
});


</script>

<div id="me_panel" class="easyui-panel" style="position:absolute;width:100%;height:100%;" data-options="border:false">
   <div id="testzone" style="width:1920px;height:1080px;position:relative;overflow:hide;">
   <div id="cm_context" class="easyui-menu" style="width:120px;">
      <div onclick="javascript:ctrlr_mapEditor._context_menu('enable')">enable</div>
      <div onclick="javascript:ctrlr_mapEditor._context_menu('disable')">disable</div>
      <div onclick="javascript:ctrlr_mapEditor._context_menu('list')">list</div>
   </div>

   <div id="cm_widget" class="easyui-menu" style="width:120px;">
      <div onclick="javascript:ctrlr_mapEditor._widget_menu('properties')">properties</div>
      <div onclick="javascript:ctrlr_mapEditor._widget_menu('delete')">delete</div>
   </div>
</div>
</div>

<div id="wn_properties" class="easyui-window" title="Objects panel" style="width:400px;height:300px;" data-options="modal:false,closed:true">
</div>

<div id="win" class="easyui-window" title="Objects panel" style="width:200px;height:500px">
   <div id="panelzone" style="width:auto;height:100%;position:relative;overflow:auto">
      <table>
      <tr>
      <td>
      <div id="jauge"  class="drag" style="width: 64px; height: 64px; border:1px solid red; background-color: gray;"></div>
      </td>
      </tr>
      <tr>
      <td>
      <div id="toggle" class="drag" style="width: 64px; height: 64px; border:1px solid red; background-color: green;"></div>
      </td>
      </tr>
      <tr>
      <td>
      <div id="switch" class="drag" style="width: 64px; height: 64px; border:1px solid red; background-color: blue;"></div>
      </td>
      </tr>
      </table>
   </div>
</div>


<div style="display:none">
   <div id="jauge_model"  mea_widget="jauge"  style="position:absolute; width: 150px; height: 150px; border:1px solid gray; background-color: gray;" class="ui-widget-content">
      <div>Jauge</div>
   </div>
   <div id="toggle_model" mea_widget="toggle" style="position:absolute; width: 100px; height: 100px; border:1px solid green; background-color: green;" class="ui-widget-content">
      <div>toggle</div>
   </div>
   <div id="switch_model" mea_widget="switch" style="position:absolute; width: 150px; height: 75px; border:1px solid blue; background-color: blue;" class="ui-widget-content">
      <div>switch</div>
   </div>
</div>
