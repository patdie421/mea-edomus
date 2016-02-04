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
//   background:#FBEC88;
   background:#FFFFCE;
}
.notover{
   background:#FFFFFF;
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

var saved = {};

function MapEditorController(container, map, propertiesPanel, actionPanel, mapContextMenu, widgetContextMenu)
{
   MapEditorController.superConstructor.call(this);

   this.container =         $('#'+container);
   this.map =               $('#'+map);
   this.toolsPanel =        false;
   this.toolsPanelState =   'closed';
   this.actionPanel =       $('#'+actionPanel);
   this.propertiesPanel =   $('#'+propertiesPanel);
   this.propertiesPanelState = 'closed';
   this.mapContextMenu =    $('#'+mapContextMenu);
   this.widgetContextMenu = $('#'+widgetContextMenu);

   this.current_zindex=this.max_zIndex(map);

   this.mea_mouse_x = -1;
   this.mea_mouse_y = -1;

   this.objid = 10;
   this.timeout = false;

   this.mapState = 'edit';

   var _this = this;

   _this.cleanexit = _this._cleanexit.bind(_this);
   _this.getmousepos_handler = _this._getmousepos_handler.bind(_this);
   _this.scroll = _this._scroll.bind(_this);
   _this.constrain = _this._constrain.bind(_this);
   _this.open_context_menu = _this._open_context_menu.bind(_this);
   _this.context_menu = _this._context_menu.bind(_this);
   _this._open_widget_menu = _this.__open_widget_menu.bind(_this);
   _this.widget_menu = _this._widget_menu.bind(_this);
   _this.widgetContextMenu.menu({onHide: function() { _this.widgetContextMenu.attr('mea_eid', false); } });
   _this.map.bind('contextmenu', _this.open_context_menu);

   _this.draggable_options = {
      delay: 250,
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

         $('body').unbind('mousemove',  _this.getmousepos_handler);

         var data   = $(this).prop('mea-widgetdata');
         var id     = data[0].value;
         var offset = _this.map.offset();
         var l      = _this.repair(d.left - offset.left);
         var t      = _this.repair(d.top - offset.top);

         data[2].value = l;
         data[3].value = t;
         data[4].value = ++_this.current_zindex;

         _this.createFromWidgetdata(data, true).draggable('enable');

         $(this).remove();

         _this._updateProperties(id);
      },

      proxy: function(source) {
         var p = $(this).clone().attr('id', '_drag').attr('id2', $(source).attr('id'));
         p.prop('_me', _this);
         p.bind('contextmenu', _this.open_widget_menu);
         p.appendTo('body');
         p.show();

         return p;
      },

      onDrag: function(e) {
         var d = e.data;
         var id=$(e.data.target).attr('mea_widget');
         _this.constrain(e, $("#"+id+"_drag"), $("#"+id+"_model"));
      }
   };

   var propertiesPanelWindow = "<div id='propertiesPanelWindow' title='Properties' style='width:400px;height:300px;'></div>";
   $('body').append(propertiesPanelWindow);
   _this.propertiesPanel = $("#propertiesPanelWindow");
   _this.propertiesPanel.window({
      top:200,
      left:600,
      modal:false,
      closed:true,
      collapsible: false,
      minimizable: false,
      maximizable: false,
      onMove: function(left, top){
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
      },
      onOpen: function() {
         _this.propertiesPanelState = 'opened';
      },
      onClose: function() {
         _this.propertiesPanelState = 'closed';
      }
   });
   _this.propertiesPanel.append("<DIV id='div_properties' style='width:100%;height:auto;position:relative'><TABLE id='tbl_properties' style='width:100%'></TABLE></DIV>");
   _this.propertiesTbl = $("#tbl_properties");
   _this.propertiesTbl.propertygrid({
      showGroup:true,
      border:false,
      scrollbarSize:0,
      rowStyler:function(index, row) {
         if(row.editor == 'empty')
            return 'background-color:pink;color:blue;font-weight:bold;';
      },
      onBeforeSelect:function(index, row) {
         var __this = this;
         if(row.group=="actions")
         {
            var rows = $(__this).propertygrid('getRows');
            var actions = {};
            var xplAction = {};
            try {
                 actions = JSON.parse(row.value);
                 xplAction = actions['xplsend']; 
            } catch(e) {
               xplAction = {};
            }
            
            _this.actionPanel.find('[name="widgetid_me"]').val(rows[0].value);
            _this.actionPanel.find('[name="action_me"]').val(row.name);
            var namesvalues_sel = _this.actionPanel.find('[name="namesvalues"]');
            var names_sel  = $("#"+_this.actionPanel.attr('id')+"_names");
            var values_sel = $("#"+_this.actionPanel.attr('id')+"_values");

            namesvalues_sel.empty();

            var data_names = [
               {
               label: 'current',
               value: 'current'
               },{
               label: 'type',
               value: 'type'
               },{
               label: 'target',
               value: 'target'
               },{
               label: 'schema',
               value: 'schema'
               }
            ];
            var data_values = [
               {
               label: 'high',
               value: 'high'
               },{
               label: 'low',
               value: 'low'
               }
            ];
            $.each(rows, function (i,val) {
               if(val.group=="variables")
               {
                  data_values.push({ label: "["+val.name+"]", value: "["+val.name+"]"});
               }
            });

            $.each(xplAction, function(i,val) {
               namesvalues_sel.append($('<option>', {
                  value: JSON.stringify({ name: i, value: val }),
                  text: i+" = "+val 
               }));

               var found=false;
               $.each(data_names, function(_i, _val) {
                  if(_val['label'] == i) {
                     found=true;
                     return false;
                  }
               });
               if(found===false)
                  data_names.push({label: i, value: i});

               found=false;
               $.each(data_values, function(_i, _val) {
                  if(_val['value'] == val) {
                     found=true;
                     return false;
                  }
               });
               if(found===false)
                  data_values.push({label: val, value: val});
            });

            names_sel.combobox({data: data_names});
            values_sel.combobox({data: data_values});

            _this.actionPanel.window('open');

            return false;
         }
      },
      onSelect:function(index, row) {
         var __this = this;
         $(__this).propertygrid('beginEdit', index);
         var ed = $(__this).propertygrid('getEditor', { index:index, field:'value'});
         $(ed.target).focus().val($(ed.target).val()); // pour positionner le curseur à la fin du champ
      },
      onBeforeEdit:function(index, row) {
         if(row.group=="actions")
            return false;
      },
      onBeginEdit:function(index) {
         var i = index;
         var __this = this;
         var ed = $(__this).propertygrid('getEditor', { index:i, field:'value'});
         var cell = $(ed.target);
         cell.bind('keyup', function(e) { c = e.keyCode || e.which(); if(c == 13) { $(__this).propertygrid('endEdit', i); } });
      },
      onEndEdit:function(index,row) {
         var __this = this;
         var i = index;
         if(row.group=="actions")
            return true;

         var ed = $(__this).propertygrid('getEditor', { index:i, field:'value'});
         var cell = $(ed.target);
         cell.unbind('keyup');
         cell.blur();
         $(__this).propertygrid('unselectAll');

         var rows = $(__this).propertygrid('getRows');
         meaWidgetsJar[rows[1].value].init(rows[0].value);
         meaWidgetsJar[rows[1].value].disabled(rows[0].value, true); 
      }
   });



   var widgetsPanelWindow = "<div id='widgetsPanelWindow' class='easyui-window' title='Widgets' style='width:150px;height:500px'> \
         <div style='width:auto;height:100%;position:relative;overflow:auto'> \
            <table id='tbl_panel'></table> \
         </div> \
      </div>";
   $('body').append(widgetsPanelWindow);
   _this.toolsPanel = $("#widgetsPanelWindow");
   _this.toolsPanel.window({
      top:100,
      left:100,
      collapsible: false,
      minimizable: false,
      maximizable: false,
      onMove: function(left, top){
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
      },
      onOpen: function() {
         _this.toolsPanelState = 'opened';
      },
      onClose: function() {
         _this.toolsPanelState = 'closed';
      }
   });

   _this.map.droppable({
      accept:'.drag',
      onDragEnter:function(e,source){
         $(source).draggable('options').cursor='auto';
         $(source).draggable('proxy').css('border','2px solid red');
         $(this).addClass('over');
         $(this).removeClass('notover');
      },

      onDragLeave:function(e,source){
         $(source).draggable('options').cursor='not-allowed';
         $(source).draggable('proxy').css('border','2px solid #ccc');
         $(this).removeClass('over');
         $(this).addClass('notover');
      },

      onDrop:function(e,source) {
         _this.objid++;
         var type=$(source).attr("id");
         var l=_this.repair($("#"+type+"_drag").offset().left);
         var t=_this.repair($("#"+type+"_drag").offset().top);
         var zi=++_this.current_zindex;
         var newid = "Widget_"+type+"_"+_this.objid;
         var offset = _this.map.offset();
         var p = $("#"+type+"_model").clone().attr('id', newid);

         $(this).append(p);

         p.css({top: t - offset.top, left: l - offset.left});
         p.css("z-index", zi);
         p.draggable(_this.draggable_options);
         p.bind('contextmenu', _this.open_widget_menu);

         mea_widgetdata = _this.newWidgetData(newid, type, l - offset.left, t - offset.top, zi, p);
         p.prop('mea-widgetdata', mea_widgetdata); 
         _this._updateProperties(newid);

         meaWidgetsJar[type].init(newid); 
         meaWidgetsJar[type].disabled(newid, true); 

         $(this).removeClass('over');
         $(this).addClass('notover');
      }
   });
}

extendClass(MapEditorController, CommonController);


MapEditorController.prototype.newWidgetData = function(newid, type, x, y, zi, p)
{
   var mea_widgetdata = [
      {"name":"id",     "value":newid, "group":"Indentification", "editor":"empty" },
      {"name":"type",   "value":type,  "group":"Indentification", "editor":"empty" },
      {"name":"x",      "value":x,     "group":"Position", "editor":"empty" },
      {"name":"y",      "value":y,     "group":"Position", "editor":"empty" },
      {"name":"zindex", "value":zi,    "group":"Position", "editor":"empty" }
   ];

   try {
      $.each(p.data().widgetparams.labels, function(i,val) {
         mea_widgetdata.push({"name":i, "value":"", "group":"labels", "editor":"text", "type":"string"});
/*
          try {
             p.find('[mea_widgetlabelname="'+i+'"]').text(val);
          }
          catch(e) {}
*/
     });
   }
   catch(e) {};
 
   try {
      $.each(p.data().widgetparams.values, function(i,val) {
         mea_widgetdata.push({"name":i, "value":"", "group":"values", "editor":"text", "type":val});
      });
   }
   catch(e) {};
 
   try {
      $.each(p.data().widgetparams.actions, function(i,val) {
         mea_widgetdata.push({"name":i, "value":val, "group":"actions", "editor":"text", "type":""});
      });
   }
   catch(e) {};
 
   try {
      $.each(p.data().widgetparams.link, function(i,val) {
         mea_widgetdata.push({"name":i, "value":val, "group":"links", "editor":"text", "type":""});
      });
   }
   catch(e) {};
 
   try {
      $.each(p.data().widgetparams.variables, function(i,val) {
         mea_widgetdata.push({"name":i, "value":val, "group":"variables", "editor":"empty", "type":""});
      });
   }
   catch(e) {};

   return mea_widgetdata;
}


MapEditorController.prototype.start = function()
{
   var _this = this;

   $(document).on("MeaCleanView", _this.cleanexit);

   var evnt = "unactivatetab_" + translationController.toLocalC('test');
   evnt=evnt.replace(/[^a-zA-Z0-9]/g,'_');
   $(document).on(evnt, function() {
      var sp = _this.propertiesPanelState;
      var st = _this.toolsPanelState;

      _this.propertiesPanel.window('close');
      _this.toolsPanel.window('close');
      _this.propertiesPanelState = sp;
      _this.toolsPanelState = st;
   });

   var evnt = "activatetab_" + translationController.toLocalC('test');
   evnt=evnt.replace(/[^a-zA-Z0-9]/g,'_');
   $(document).on(evnt, function() {
      if(_this.propertiesPanelState=='opened')
         _this.propertiesPanel.window('open');
      if(_this.toolsPanelState=='opened')
         _this.toolsPanel.window('open');
   });
}


MapEditorController.prototype._cleanexit = function()
{
   var _this = this;

   $(document).off("MeaCleanView", _this.cleanexit);

   var evnt = "unactivatetab_" + translationController.toLocalC('test');
   evnt=evnt.replace(/[^a-zA-Z0-9]/g,'_');
   $(document).off(evnt);

   var evnt = "activatetab_" + translationController.toLocalC('test');
   evnt=evnt.replace(/[^a-zA-Z0-9]/g,'_');
   $(document).off(evnt);

   _this.toolsPanel.window('close');
   _this.toolsPanel.window('destroy');
   _this.propertiesPanel.window('close');
   _this.propertiesPanel.window('destroy');

   return true;
}


MapEditorController.prototype.saveTo = function(s)
{
   var _this = this;

   s['width']=_this.map.width();
   s['height']=_this.map.height();

   s['background-color']=false;
   s['background-image']=false;

   s['widgets']={};

   $('div[id^="Widget_"]').each(function(){
      var data = JSON.parse(JSON.stringify($(this).prop('mea-widgetdata')));
      s['widgets'][data[0].value]=data; 
   });
}


MapEditorController.prototype.createFromWidgetdata = function(obj, d)
{
   var _this = this;

   var id   = obj[0].value;
   var type = obj[1].value;
   var x    = obj[2].value;
   var y    = obj[3].value;
   var zi   = obj[4].value;

   var offset = _this.map.offset();

   var p = $("#"+type+"_model").clone().attr('id', id);

   _this.map.append(p);

   p.css({top: y, left: x});
   p.css("z-index", zi);
   p.prop('_me', _this);
   p.bind('contextmenu', _this.open_widget_menu);
   p.draggable(_this.draggable_options);
   p.draggable('disable');
   p.prop('mea-widgetdata', obj);

   meaWidgetsJar[type].init(id);
   meaWidgetsJar[type].disabled(id, d);

   return p;
}


MapEditorController.prototype.loadFrom = function(s)
{
   var _this = this;

   $('div[id^="Widget_"]').each(function(){
      $(this).empty();
      $(this).remove();
   });

   _this.map.width(s['width']);
   _this.map.height(s['height']);

   _this.objid=0;
   _this.current_zindex=0;
   $.each(s['widgets'], function(i, obj) {
      var id   = obj[0].value;
      var zi   = obj[4].value;

      _this.createFromWidgetdata(obj, false);

      var _objid = id.match(/\d+$/);
      if(_objid)
         _objid=parseInt(_objid[0]);
      else
         _objid=0;
      if(_objid > _this.objid)
         _this.objid = _objid;
      if(zi > _this.current_zindex)
         _this.current_zindex=zi;
   });
}


MapEditorController.prototype._context_menu = function(action)
{
   var _this = this;

   switch(action)
   {
      case 'load':
         _this.loadFrom(saved);
         break;

      case 'save':
         saved = {};
         _this.saveTo(saved);
         break;

      case 'toggle':
         var item = "";
         if(_this.mapState == 'edit') {
            _this.mapState = 'view';
            item = _this.mapContextMenu.menu('findItem', 'map view');
            _this.mapContextMenu.menu('setText', { target: item.target, text: 'map edit'});
            var _tmp = {};
            _this.toolsPanel.window('close');
            _this.propertiesPanel.window('close');
            _this.saveTo(_tmp);
            _this.loadFrom(_tmp);
            tmp = null;
         }
         else {
            _this.mapState = 'edit';
            item = _this.mapContextMenu.menu('findItem','map edit');
            _this.mapContextMenu.menu('setText', { target:item.target, text: 'map view'});
            _this.toolsPanel.window('open');
            $('div[id^="Widget_"]').each(function(){
               var data = $(this).prop('mea-widgetdata');
               meaWidgetsJar[data[1].value].init(data[0].value, true);
               meaWidgetsJar[data[1].value].disabled(data[0].value, true);
               $(this).draggable('enable');
            });
         }
         break;
   }
}


MapEditorController.prototype._updateProperties = function(id)
{
   var _this = this;

   var _id = $("#"+id);

   var y  = parseInt(_id.css("top"));
   var x  = parseInt(_id.css("left"));
   var zi = parseInt(_id.css("z-index"));

   var data = _id.prop('mea-widgetdata');
   _this.propertiesTbl.propertygrid({data: data});
   data[2].value = x;
   data[3].value = y;
   data[4].value = zi;

   function _resize()
   {
      var h = $('#div_properties').height();
      if(h>100)
      {
         _this.propertiesPanel.window('resize', { height: h+40 });
         return true;
      }
      setTimeout( _resize, 25);
      return false;
   }

   _resize();
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
         return true;

      case 'properties':
         _this.propertiesPanel.window('open');
         return true;
   }
}


MapEditorController.prototype.max_zIndex = function(div)
{
   var _this = this;
   var zi = 0;
   var max = 0;

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
   var offset_map = _this.map.offset();

   var l_min = offset.left;
   var t_min = offset.top;

   var l_max = 0;
   if(_this.container.outerWidth() <= _this.map.outerWidth())
      l_max = _this.container.outerWidth()+offset.left;
   else
      l_max = _this.map.outerWidth()+offset.left;

   var t_max = 0;
   if(_this.container.outerHeight() <= _this.map.outerHeight())
      t_max = _this.container.outerHeight()+offset.top;
   else
      t_max = _this.map.outerHeight()+offset.top;

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

   id = $(_this).attr('id2');
   if(!id)
      id = $(_this).attr('id');

   _me._open_widget_menu(id, e.pageX, e.pageY);

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
//         var id=$(e.data.target).attr('id');
//         _this.constrain(e, $("#"+id+"_drag"), $("#"+id+"_model"));
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


MapEditorController.prototype._xplEditorCancel = function()
{
   var _this = this;

   _this.actionPanel.window('close');
}


MapEditorController.prototype._xplEditorOk = function()
{
   var _this = this;
   var data = {};

   var id = _this.actionPanel.attr('id');
   var namesvalues_sel = _this.actionPanel.find('[name="namesvalues"] > option');
   namesvalues_sel.each(function() {
      var namevalue = JSON.parse($(this).val());
      data[namevalue.name]=namevalue.value;
   });

   var widgetid = _this.actionPanel.find('[name="widgetid_me"]').val();
   var widgetdata = $("#"+widgetid).prop('mea-widgetdata');

   action = _this.actionPanel.find('[name="action_me"]').val();

   $.each(widgetdata, function(i, val) {
      if(val.name === action)
      {
         val.value = JSON.stringify({ "xplsend" : data });
         return false;
      }
   });

   _this._updateProperties(widgetid);
   _this.actionPanel.window(close);
}


MapEditorController.prototype._xplEditorDown = function()
{
   var _this = this;
   var id = _this.actionPanel.attr('id');

   var name = $("#"+id+"_names").combobox('getText');
   if(name)
      name=name.trim();
   var value = $("#"+id+"_values").combobox('getText');
   if(value)
      value=value.trim();

   if(name && name.length && value && value.length)
   {
      var namesvalues_sel = _this.actionPanel.find('[name="namesvalues"] > option');
      var found = false;
      namesvalues_sel.each(function() {
         var namevalue = JSON.parse($(this).val());
         if(namevalue.name == name) {
            $(this).val(JSON.stringify({ name: name, value: value }));
            $(this).text(name+" = "+value);
            found = true;
            return false;
         }
      });
      if(found === false)
      {
         _this.actionPanel.find('[name="namesvalues"]').append($('<option>', {
            value: JSON.stringify({ name: name, value: value }),
            text: name+" = "+value
         }));
      }
   } 
}


MapEditorController.prototype._xplEditorUp = function()
{
   var _this = this;
   var sel = _this.actionPanel.find('[name="namesvalues"] option:selected');
   var id = _this.actionPanel.attr('id');
   if(sel)
   {
      var namevalue = JSON.parse(sel.val());
      $("#"+id+"_names").combobox('setValue',namevalue.name);
      $("#"+id+"_values").combobox('setValue',namevalue.value);
   }
   sel.remove();
}


MapEditorController.prototype.loadWidgets = function(list)
{
   var _this = this;

   function load_widgets(list, i)
   {
      $.getScript(list[i], function(data, textStatus, jqxhr) {
         if(jqxhr.status == 200)
            console.log(list[i]+": loaded");
         else
            console.log(list[i]+": error - "+textStatus);
         i=i-1;
         if(i<0)
         {
            $.each(meaWidgetsJar, function(i,obj) {
               $('#tbl_panel').append('<tr><td>'+obj.getHtmlIcon()+'</td><tr>');
               $('#widgets_container').append(obj.getHtml());
               ctrlr_mapEditor.addWidget(obj.getType());

               $.each(obj.getFormaters(), function(i,val) {
                  meaFormaters[i]=val;
               });
            });
         }
         else
            load_widgets(list, i);
      });
   }

   var i=list.length-1;

   load_widgets(list, i);
}


var i=0;
function simu()
{
   ++i;
   setTimeout(simu, 5000);

   $.each($("#map_me").find('label[name="A1"]'), function() {
     var _formater = $(this).attr('mea_valueformater');
     if(_formater)
        $(this).text(meaFormaters[_formater](i));
     else
        $(this).text(i);
   });

   $.each($("#map_me").find('input[name="A1"]'), function() {
     var _formater = $(this).attr('mea_valueformater');
     if(_formater)
        $(this).val(meaFormaters[_formater](i));
     else
        $(this).val(i);
   });

   $.each($("#map_me").find('div[name="A1"]'), function() {
     var _formater = $(this).attr('mea_valueformater');
     if(_formater)
     {
        meaFormaters[_formater](i, $(this));
     }
     else
        $(this).html(i);
   });

   $("#map_me").find('label[name="A2"]').text(i*2);
   $("#map_me").find('input[name="A2"]').val(i*2);
   $.each($("#map_me").find('div[name="A2"]'), function() {
     var _formater = $(this).attr('mea_valueformater');
     if(_formater)
     {
        meaFormaters[_formater](i*2, $(this));
     }
     else
        $(this).html(i);
   });

}


function newMap(newWinId, mapId) {
   var newWin = $("#"+newWinId);
   var map =  $("#"+mapId);

   $('div[id^="Widget_"]').each(function(){
      $(this).empty();
      $(this).remove();
   });

   newWin.window('open');

   $("#select_"+newWinId).combobox({
      onChange: function(n,o) {
         if(n=='custom') {
            $("#input_width_"+newWinId).textbox({disabled: false});
            $("#input_height_"+newWinId).textbox({disabled: false});
         }
         else {
            n=JSON.parse(n);
            $("#input_width_"+newWinId).textbox({disabled: true, value: n.width});
            $("#input_height_"+newWinId).textbox({disabled: true, value: n.height});
         }
      },
      value: 'custom'
   });

   $("#button_cancel_ft_"+newWinId).off('click').on('click', function() { newWin.window('close'); });
   $("#button_ok_ft_"+newWinId).off('click').on('click', function() {
      var w = parseInt($("#input_width_"+newWinId).textbox("getText"));
      var h = parseInt($("#input_height_"+newWinId).textbox("getText"));

     map.width(w);
     map.height(h);

     newWin.window('close');
   });
}


jQuery(document).ready(function() {
   var list = [
      "../widgets/meawidget_lampe.js",
      "../widgets/meawidget_slider.js",
      "../widgets/meawidget_button.js"
   ];

   ctrlr_mapEditor = new MapEditorController(
      "panel_me",
      "map_me",
      "properties_win_me",
      "actions_win_me",
      "map_cm_me",
      "widget_cm_me");
   ctrlr_mapEditor.linkToTranslationController(translationController); 
   ctrlr_mapEditor.linkToCredentialController(credentialController); 

   var xplEditorUp     = ctrlr_mapEditor._xplEditorUp.bind(ctrlr_mapEditor);
   var xplEditorDown   = ctrlr_mapEditor._xplEditorDown.bind(ctrlr_mapEditor);
   var xplEditorOk     = ctrlr_mapEditor._xplEditorOk.bind(ctrlr_mapEditor);
   var xplEditorCancel = ctrlr_mapEditor._xplEditorCancel.bind(ctrlr_mapEditor);

   ctrlr_mapEditor.loadWidgets(list);

   ctrlr_mapEditor.start();

   setTimeout(simu, 5000);

   $("#button_up_actions_win_me").on('click', xplEditorUp);
   $("#button_down_actions_win_me").on('click', xplEditorDown);
   $("#button_ok_ft_action_win_me").on('click', xplEditorOk);
   $("#button_cancel_ft_action_win_me").on('click', xplEditorCancel);
/*
   $(document).mouseleave(function(e){console.log('out')});
   $(document).mouseenter(function(e){console.log('in')});
*/
});


</script>

<div id="panel_me" class="easyui-panel" style="position:absolute;width:100%;height:100%;overflow:scroll;background:#EEEEEE" data-options="border:false">
   <div id="map_me" class="notover" style="width:1920px;height:1080px;position:relative;overflow:hidden;border:1px solid #555555">
   </div>
   <div id="widgets_container" style="display:none"></div>
</div>

<div id="map_cm_me" class="easyui-menu" style="width:180px;display:hidden;">
<!--   <div onclick="javascript:ctrlr_mapEditor._context_menu('new')">new</div> -->
   <div onclick="javascript:newMap('new_win_me', 'map_me')">new</div>
   <div onclick="javascript:ctrlr_mapEditor._context_menu('load')">load</div>
   <div onclick="javascript:ctrlr_mapEditor._context_menu('save')">save</div>
   <div onclick="javascript:ctrlr_mapEditor._context_menu('saveas')">save as</div>
   <div class="menu-sep"></div>
   <div onclick="javascript:ctrlr_mapEditor._context_menu('backgroundc')">background color</div>
   <div onclick="javascript:ctrlr_mapEditor._context_menu('backgroundi')">background image</div>
   <div class="menu-sep"></div>
   <div name="toggle" onclick="javascript:ctrlr_mapEditor._context_menu('toggle')">map view</div>
</div>

<div id="widget_cm_me" class="easyui-menu" style="width:120px;display:hidden">
   <div onclick="javascript:ctrlr_mapEditor._widget_menu('properties')">properties</div>
   <div onclick="javascript:ctrlr_mapEditor._widget_menu('delete')">delete</div>
</div>

<div id="all_windows" style="display:none"></div>


<div id="new_win_me" class="easyui-window" style="position:relative;width:300px;height:180px;overflow:hidden" data-options="title:'map size',modal:true,closed:true,footer:'#ft_new_win_me'">
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


<div id="actions_win_me" class="easyui-window" style="position:relative;width:500px;height:350px;overflow:hidden" data-options="title:'xPL send parameters',modal:true,closed:true,footer:'#ft_action_win_me'">
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
            <select id="actions_win_me_namesvalues" name="namesvalues" id="xpl_me" size="10" style="text-align:center;width:80%;font-family:verdana,helvetica,arial,sans-serif;font-size:12px;">
            </select>
         </td>
      </tr>
   </table>
   <input id="actions_win_me_widget" name="widgetid_me" type="hidden">
   <input id="actions_win_me_action" name="action_me" type="hidden">
</div>
<div id="ft_action_win_me" style="text-align:right;padding:5px;display:hidden">
      <a id="button_ok_ft_action_win_me"      href="javascript:void(0)" class="easyui-linkbutton" style="width=50px;" data-options="iconCls:'icon-ok'"><?php mea_toLocalC('ok'); ?></a>
      <a id="button_cancel_ft_action_win_me", href="javascript:void(0)" class="easyui-linkbutton" style="width=50px;" data-options="iconCls:'icon-cancel'"><?php mea_toLocalC('cancel'); ?></a>
</div>
