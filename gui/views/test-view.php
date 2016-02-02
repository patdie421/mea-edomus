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

var saved = {};

function MapEditorController(container, map, propertiesPanel, mapContextMenu, widgetContextMenu)
{
   MapEditorController.superConstructor.call(this);

   this.container =         $('#'+container);
   this.map =               $('#'+map);
   this.toolsPanel =        false;
   this.toolsPanelState =   'closed';
   this.propertiesPanel =   $('#'+propertiesPanel);
   this.propertiesPanelState = 'closed';
   this.mapContextMenu =    $('#'+mapContextMenu);
   this.widgetContextMenu = $('#'+widgetContextMenu);

   this.current_zindex=this.max_zIndex(map);

   this.mea_mouse_x = -1;
   this.mea_mouse_y = -1;

   this.objid = 10;
   this.timeout = false;

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

         var offset = _this.map.offset();
         var l      = _this.repair(d.left - offset.left);
         var t      = _this.repair(d.top - offset.top);
         var data   = $(this).prop('mea-widgetdata');

         var p      = $(this).clone();

         p.css({top: t, left: l}).draggable(_this.draggable_options);
         p.css("z-index", ++_this.current_zindex);
         p.prop('mea-widgetdata', data);
         p.bind('contextmenu', _this.open_widget_menu);
         p.prop('_me', _this);
         _this.map.append(p);
         p.show();
         $(this).remove();
         _this._updateProperties(p.attr('id'));
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
         if(row.group=="actions")
            return false;
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

         var l=_this.repair($("#"+type+"_drag").offset().left);
         var t=_this.repair($("#"+type+"_drag").offset().top);
         var zi=++_this.current_zindex;

         p.css({top: t - offset.top, left: l - offset.left}).draggable(_this.draggable_options);
         p.css("z-index", zi);
         p.bind('contextmenu', _this.open_widget_menu);

         var mea_widgetdata = [
            {"name":"id",     "value":newid,            "group":"Indentification", "editor":"empty" },
            {"name":"type",   "value":type,             "group":"Indentification", "editor":"empty" },
            {"name":"x",      "value":t - offset.top,   "group":"Position", "editor":"empty" },
            {"name":"y",      "value":l - offset.left,  "group":"Position", "editor":"empty" },
            {"name":"zindex", "value":zi,               "group":"Position", "editor":"empty" }
         ];

         try {
            $.each(p.data().widgetparams.labels, function(i,val) {
               mea_widgetdata.push({"name":i, "value":"", "group":"labels", "editor":"text", "type":"string"});
               try {
                  p.find('[mea_widgetlabelname="'+i+'"]').text(val);
               }
               catch(e) {}
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

         $("#"+newid).prop('mea-widgetdata', mea_widgetdata); 
         _this._updateProperties(newid);

         meaWidgetsJar[type].init(newid); 

         $(this).removeClass('over');
      }
   });
}

extendClass(MapEditorController, CommonController);


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


MapEditorController.prototype._context_menu = function(action)
{
   var _this = this;

   switch(action)
   {
      case 'load':

         $('div[id^="widget_"]').each(function(){
            $(this).empty();
            $(this).remove();
         });

         _this.objid=0;
         _this.current_zindex=0;
         $.each(saved, function(i, obj) {

            var id   = obj[0].value;
            var type = obj[1].value;
            var x    = obj[2].value;
            var y    = obj[3].value;
            var zi   = obj[4].value;

            var offset = _this.map.offset();

            var p = $("#"+type+"_model").clone().attr('id', id);

            _this.map.append(p);

            p.css({top: y, left: x}); //.draggable(options);
            p.css("z-index", zi);
            p.bind('contextmenu', _this.open_widget_menu);
            p.draggable(_this.draggable_options);
            p.draggable('disable');
            p.prop('mea-widgetdata', obj);

            meaWidgetsJar[type].init(id);

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
         break;

      case 'save':
         saved = {};
         $('div[id^="widget_"]').each(function(){
            var data = JSON.parse(JSON.stringify($(this).prop('mea-widgetdata')));
            saved[data[0].value]=data; 
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
         return false;
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

   console.log("type=",type);

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


var i=0;
function simu()
{
   ++i;
   setTimeout(simu, 5000);

   $.each($("#testzone").find('label[name="A1"]'), function() {
     var _formater = $(this).attr('mea_valueformater');
     if(_formater)
        $(this).text(meaFormaters[_formater](i));
     else
        $(this).text(i);
   });

   $.each($("#testzone").find('input[name="A1"]'), function() {
     var _formater = $(this).attr('mea_valueformater');
     if(_formater)
        $(this).val(meaFormaters[_formater](i));
     else
        $(this).val(i);
   });

   $.each($("#testzone").find('div[name="A1"]'), function() {
     var _formater = $(this).attr('mea_valueformater');
     if(_formater)
     {
        meaFormaters[_formater](i, $(this));
     }
     else
        $(this).html(i);
   });

   $("#testzone").find('label[name="A2"]').text(i*2);
   $("#testzone").find('input[name="A2"]').val(i*2);
   $.each($("#testzone").find('div[name="A2"]'), function() {
     var _formater = $(this).attr('mea_valueformater');
     if(_formater)
     {
        meaFormaters[_formater](i*2, $(this));
     }
     else
        $(this).html(i);
   });

}


jQuery(document).ready(function() {
   ctrlr_mapEditor = new MapEditorController(
      "me_panel",
      "testzone",
      "wn_properties",
      "cm_context",
      "cm_widget");
   ctrlr_mapEditor.linkToTranslationController(translationController); 
   ctrlr_mapEditor.linkToCredentialController(credentialController); 

   ctrlr_mapEditor.start();

   var list = [
      "../widgets/meawidget_lampe.js",
      "../widgets/meawidget_slider.js",
      "../widgets/meawidget_button.js"
   ];
   i=list.length-1;
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
   load_widgets(list, i);

   setTimeout(simu, 5000);

/*
   $(document).mouseleave(function(e){console.log('out')});
   $(document).mouseenter(function(e){console.log('in')});
*/
});


</script>

<div id="me_panel" class="easyui-panel" style="position:absolute;width:100%;height:100%;overflow:scroll" data-options="border:false">
   <div id="testzone" style="width:1920px;height:1080px;position:relative;overflow:hidden;">
   </div>
</div>

<div id="cm_context" class="easyui-menu" style="width:120px;display:hidden;">
   <div onclick="javascript:ctrlr_mapEditor._context_menu('enable')">enable</div>
   <div onclick="javascript:ctrlr_mapEditor._context_menu('disable')">disable</div>
   <div onclick="javascript:ctrlr_mapEditor._context_menu('save')">save</div>
   <div onclick="javascript:ctrlr_mapEditor._context_menu('load')">load</div>
</div>

<div id="cm_widget" class="easyui-menu" style="width:120px;display:hidden">
   <div onclick="javascript:ctrlr_mapEditor._widget_menu('properties')">properties</div>
   <div onclick="javascript:ctrlr_mapEditor._widget_menu('delete')">delete</div>
</div>

<div id="widgets_container" style="display:none"></div>

<div id="all_windows" style="display:none"></div>

<div id="window_me" class="easyui-window" style="position:relative;width:500px;height:350px;overflow:hidden" data-options="title:'xPL send parameters',modal:true,footer:'#ft'">
   <table cellpadding="5" style="width:100%">
      <tr>
         <td align="center">Name</td>
         <td></td>
         <td align="center">Value</td>
      </tr>
      <tr>
         <td align="center">
            <select class="easyui-combobox" name="state" style="width:200px;">
               <option value="current">current</option>
               <option value="current">type</option>
               <option value="source">target</option>
               <option value="schema">schema</option>
            </select>
         </td>
         <td>=</td>
         <td align="center">
            <select class="easyui-combobox" name="state" style="width:200px;">
            </select>
         </td>
      </tr>
      <tr>
         <td align="center" colspan="3">
            <a id="button_up_me"    href="javascript:void(0)" class="easyui-linkbutton" style="width=50px;" data-options="iconCls:'icon-meauparrow'"></a>
            <a id="button_down_me", href="javascript:void(0)" class="easyui-linkbutton" style="width=50px;" data-options="iconCls:'icon-meadownarrow'"></a>
         </td>
      </td>
      <tr>
         <td align="center" colspan="3">
            <select name="xpl_me" id="xpl_me" size="10" style="text-align:center;width:80%;font-family:verdana,helvetica,arial,sans-serif;font-size:12px;">
               <option value='"current" = "high"'>"current" = "high"</option>
               <option value='"current" = "high"'>"current" = "low"</option>
            </select>
         </td>
      </tr>
   </table>

</div>

<div id="ft" style="text-align:right;padding:5px">
      <a id="button_ok_me"      href="javascript:void(0)" class="easyui-linkbutton" style="width=50px;" data-options="iconCls:'icon-ok'"><?php mea_toLocalC('ok'); ?></a>
      <a id="button_cancel_me", href="javascript:void(0)" class="easyui-linkbutton" style="width=50px;" data-options="iconCls:'icon-cancel'"><?php mea_toLocalC('cancel'); ?></a>
</div>
