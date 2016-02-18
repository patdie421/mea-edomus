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
   background:#FFFFCE;
}
.notover{
   background:#FFFFFF;
}
::-webkit-scrollbar-track {
    -webkit-box-shadow: inset 0 0 6px rgba(0,0,0,0.3); 
    -webkit-border-radius: 10px;
    border-radius: 10px;
}
::-webkit-scrollbar:vertical {
    width: 8px;
}
::-webkit-scrollbar:horizontal {
    height: 8px;
}
::-webkit-scrollbar-track {
    -webkit-box-shadow: inset 0 0 6px rgba(0,0,0,0.3); 
    -webkit-border-radius: 10px;
    border-radius: 10px;
}
::-webkit-scrollbar-thumb {
    -webkit-border-radius: 10px;
    border-radius: 10px;
    -webkit-box-shadow: inset 0 0 6px rgba(0,0,0,0.5); 
}
::-webkit-scrollbar-thumb:window-inactive {
	background: rgba(255,0,0,0.4); 
}
::-webkit-scrollbar:vertical {
    width: 8px;
}
::-webkit-scrollbar:horizontal {
    height: 8px;
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
$.extend($.fn.validatebox.defaults.rules, {
   device_name: {
      validator: function(value, param){
         return checkRegexp( value, /^[0-9a-z]*$/);
      },
      message: "caractères dans l'intervalle [a-z0-9] uniquement."
   }
});


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

function MapEditorController(container, map, propertiesPanel, actionPanel, newPanel, mapContextMenu, widgetContextMenu)
{
   MapEditorController.superConstructor.call(this);

   this.container =         $('#'+container);
   this.map =               $('#'+map);
   this.actionPanel =       $('#'+actionPanel);
   this.newPanel =          $('#'+newPanel);
   this.propertiesPanel =   $('#'+propertiesPanel);
   this.mapContextMenu =    $('#'+mapContextMenu);
   this.widgetContextMenu = $('#'+widgetContextMenu);

   this.toolsPanel = false;
   this.toolsPanelState = 'closed';
   this.propertiesPanelState = 'closed';

   this.current_file=false;
   this.ctrlr_filechooser = new FileChooserController("#"+container);
   this.ctrlr_fileuploaderchooser = new FileChooserUploaderController("#"+container);

   this.saved = {};

   this.imagepath = '/images';
   this.bgimage = false;
   this.bgcolor = false;

   this.current_zindex=this.max_zIndex(map);
   this.grid = 10;
   this.mea_mouse_x = -1;
   this.mea_mouse_y = -1;
   this.dragDropEntered = false;
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
//      delay: 250,
      onBeforeDrag: function(e)
      {
         _this._updateProperties($(this).attr('id'));

         zindex = $(this).css("z-index");
         if(zindex != _this.current_zindex)
         $(this).css("z-index", ++_this.current_zindex);
      },

      onStartDrag: function(e)
      {
         $(this).hide();
         $('body').append($(this));
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
         var l      = d.left - offset.left - 1;
         var t      = d.top - offset.top - 1;

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

   _this.propertiesPanel_init();
   _this.widgetsPanel_init();
}

extendClass(MapEditorController, CommonController);


MapEditorController.prototype.widgetsPanel_init = function()
{
   var _this = this;

   var widgetsPanelId = "widgetsPanel_win_me";
   $('#accordion_'+widgetsPanelId).accordion({
      fit: true,
      animate:false,
      border:false
   });
   _this.toolsPanel = $("#"+widgetsPanelId);
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
         $(this).addClass('over');
         $(this).removeClass('notover');
         _this.dragDropEntered = true;
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
         var l=$("#"+type+"_drag").offset().left-1;
         var t=$("#"+type+"_drag").offset().top-1;
         var zi=++_this.current_zindex;
         var newid = "Widget_"+type+"_"+_this.objid;
         var offset = _this.map.offset();


         var p = $("#"+type+"_model").clone().attr('id', newid);
         var mea_widgetdata = _this.newWidgetData(newid, type, l - offset.left, t - offset.top, zi, p);
         p.css({top: t - offset.top, left: l - offset.left});
         p.css("z-index", zi);
         p.prop('_me', _this);
         p.bind('contextmenu', _this.open_widget_menu);
         p.prop('mea-widgetdata', mea_widgetdata);
         p.draggable(_this.draggable_options);

         $(this).append(p);

         _this._updateProperties(newid);

         meaWidgetsJar[type].init(newid); 
         meaWidgetsJar[type].disabled(newid, true); 

         $(this).removeClass('over');
         $(this).addClass('notover');
      }
   });
}


MapEditorController.prototype.propertiesPanel_init = function()
{
   var _this = this;

   var propertiesPanelWindow = "<div id='properties_win_me' class='win_me' title='Properties' style='width:400px;height:300px;'></div>";
   $('body').append(propertiesPanelWindow);
   _this.propertiesPanel = $("#properties_win_me");
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
//            var namesvalues_sel = _this.actionPanel.find('[name="namesvalues"]');
            var id = _this.actionPanel.attr('id');
//            var namesvalues_sel  = $("#"+id+"_namesvalues");
            var namesvalues2_sel  = $("#"+id+"_namesvalues2");
            var names_sel        = $("#"+id+"_names");
            var values_sel       = $("#"+id+"_values");

//            namesvalues_sel.empty();

            namesvalues2_sel.datalist({
               valueField: 'f1',
               textField:  'f2',
               lines: false,
               width:380,
               height:160,
               align:'center',
            });

            var data_names = [
               {
               label: 'device',
               value: 'device'
               },{
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
               label: 'input',
               value: 'input'
               },{
               label: 'output',
               value: 'output'
               },{
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

            var namesvalues2_data = [];
            $.each(xplAction, function(i,val) {
/*
               namesvalues_sel.append($('<option>', {
                  value: JSON.stringify({ name: i, value: val }),
                  text: i+" = "+val 
               }));
*/
               namesvalues2_data.push({f1: [i,val], f2:i+" = "+val});

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
            namesvalues2_sel.datalist({data: namesvalues2_data});

            var names_keyhandler = {
 	       up: function(e){},
	       down: function(e){},
	       left: function(e){},
	       right: function(e){},
               query: function(q,e){},
               enter: function(e) {
                  if(names_sel.combobox('isValid')) {
                     names_sel.combobox('hidePanel');
                     values_sel.textbox('textbox').focus();
                  }
               }
            };
            names_sel.combobox({data: data_names, keyHandler:names_keyhandler, required: true, validType: 'device_name' });
            names_sel.combobox('textbox').on('keydown', function(e){
               if(e.keyCode == 187 || e.keyCode == 9)
               {
                  e.preventDefault();
                  if(names_sel.combobox('isValid')) {
                     names_sel.combobox('hidePanel');
                     values_sel.textbox('textbox').focus();
                  }
               }
	    });

            var values_keyhandler = {
 	       up: function(e){},
	       down: function(e){},
	       left: function(e){},
	       right: function(e){},
               query: function(q,e){},
               enter: function(e) {
                  if(values_sel.combobox('isValid')) {
                     values_sel.combobox('hidePanel');
                     _this._xplEditorDown();
                     names_sel.combobox('setText','');
                     values_sel.combobox('setText','');
                     names_sel.textbox('textbox').focus();
                  }
               }
            };
            values_sel.combobox({data: data_values, required: true, keyHandler:values_keyhandler });

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
         meaWidgetsJar[rows[1].value].update(row);
         meaWidgetsJar[rows[1].value].init(rows[0].value);
         meaWidgetsJar[rows[1].value].disabled(rows[0].value, true); 
      }
   });
}


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
         mea_widgetdata.push({"name":i, "value":val, "group":"labels", "editor":"text", "type":"string"});
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
      _this.actionPanel.window('close');
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
   _this.newPanel.window('close');
   _this.newPanel.window('destroy');
   _this.actionPanel.window('close');
   _this.actionPanel.window('destroy');

   return true;
}


MapEditorController.prototype.saveTo = function(s)
{
   var _this = this;

   s['width']=_this.map.width();
   s['height']=_this.map.height();

   s['background-color']=_this.bgcolor;
   s['bgimage']=_this.bgimage;
   s['grid']=_this.grid;

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

   if(d === true)
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

   _this.propertiesPanel.window('close');
   _this.toolsPanel.window('close');

   $('div[id^="Widget_"]').each(function(){
      $(this).empty();
      $(this).remove();
   });

   _this.map.width(s['width']);
   _this.map.height(s['height']);
   _this.bgimage=s['bgimage'];
    if(s['grid'])
      _this.grid=s['grid'];

   if(s['bgcolor'])
      _this.bgcolor=s['bgcolor'];
   else
      _this.bgcolor="#FFFFFF";
   _this.map.css("background-color", _this.bgcolor);

   console.log("ICI:"+_this.bgimage);
   if(_this.bgimage)
   {
      _this.map.css("background", "url('"+_this.imagepath+"/"+_this.bgimage+"') no-repeat");
      _this.map.css("background-size", "cover");
   }
   else
   {
      _this.map.css("background", '');
      _this.map.css("background-size", '');
   }

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

   _this.automatorSendAllInputs();
}


MapEditorController.prototype.load_map = function(name, type, checkflag)
{
   var _this = this;
   saved = false;
   var __load_map = function(name, type, checkflag)
   {
      _this.current_file=name;
      if(checkflag === true)
         return;

      $.get("models/get_file.php", { type: type, name: name }, function(response) {
         if(response.iserror===false)
         {
            try {
               var item = _this.mapContextMenu.menu('getItem', $('#'+_this.mapContextMenu.attr('id')+'_mode')[0]);
               _this.mapState = 'view';
               _this.mapContextMenu.menu('setText', { target: item.target, text: 'to edition mode'});

               _this.loadFrom(JSON.parse(response.file));

            }
            catch(e) {};
         }
         else
         {
            console.log(JSON.stringify(response));
         }
      }).done(function() {
      }).fail(function(jqXHR, textStatus, errorThrown) {
         $.messager.show({
            title:_this._toLocalC('error')+_this._localDoubleDot(),
            msg: _this._toLocalC("communication error")+' ('+textStatus+')'
         });
      });
   }

   return __load_map(name, type, checkflag);
};


MapEditorController.prototype.automatorSendAllInputs = function()
{
   var _this = this;

   $.post("CMD/automator.php", { cmnd: "sendallinputs" }, function(response) {
//      console.log(JSON.stringify(response));
      if(response.iserror===false)
      {
      }
      else
      {
      }
   }).done(function() {
   }).fail(function(jqXHR, textStatus, errorThrown) {
      $.messager.show({
         title:_this._toLocalC('error')+_this._localDoubleDot(),
         msg: _this._toLocalC("communication error")+' ('+textStatus+')'
      });
   });
}


MapEditorController.prototype.save_map = function(name, type, checkflag)
{
   var _this = this;

   $.post("models/put_file.php", { type: type, name: name, file: JSON.stringify(_this.saved) }, function(response) {
   if(response.iserror===false)
      _this.current_file=name;
   else
      $.messager.alert(_this._toLocalC('error'), _this._toLocalC("Can't save file")+ " (" + _this._toLocal('server message')+_this._localDoubleDot()+response.errMsg+")", 'error');
   });
}


MapEditorController.prototype.delete_map = function(name, type, checkflag)
{
   this._delete_map(name, type);
}


MapEditorController.prototype._delete_map = function(name, type)
{
   var _this = this;

   var _delete_rules = function() {
      $.post("models/del_file.php", { name: name, type: type }, function(response) {

         if(response.iserror === false)
         {
         }
         else
         {
            $.messager.alert(_this._toLocalC('error'),
                             _this._toLocalC("can't delete map")+_this._localDoubleDot()+response.errMsg,
                             'error');
         }
      }).done(function() {
      }).fail(function(jqXHR, textStatus, errorThrown) {
         $.messager.show({
            title:_this._toLocalC('error')+_this._localDoubleDot(),
            msg: _this._toLocalC("communication error")+' ('+textStatus+')'
         });
      });
   }

    $.messager.defaults.ok = _this._toLocalC('Yes');
    $.messager.defaults.cancel = _this._toLocalC('No');
    $.messager.confirm(_this._toLocalC('deleting')+' map ...',_this._toLocalC('are you sure you want to delete')+' "'+name+'"'+_this._localDoubleDot(), function(r)
    {
       if(r)
          _delete_rules(name);
    });
}


function isNormalInteger(str) {
    return /^\+?(0|[1-9]\d*)$/.test(str);
}


MapEditorController.prototype.newMap = function() {
   var _this = this;

   var newWin = _this.newPanel;
   var newWinId = newWin.attr('id');
   var map =  _this.map;

   $("#input_width_"+newWinId).textbox({disabled: false});
   $("#input_height_"+newWinId).textbox({disabled: false});

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

      var w = $("#input_width_"+newWinId).textbox("getText");
      var h = $("#input_height_"+newWinId).textbox("getText");

//      console.log(w+" x "+h);

      if(!isNormalInteger(w) || !isNormalInteger(h))
         return;
      var w = parseInt(w);
      var h = parseInt(h);
     if(!w || !h)
        return;

     //min cga : 320x200
     if(w<320)
        w=320;
     if(h<200)
        h=200;

     //max 8k : 7680×4320
     if(w>7680)
        w=7680;
     if(h>4320)
        h=4320;

     _this.current_file=false;
     _this.bgimage=false;

     map.css("background",'');
     map.css("background-size",'');
     map.css("background-color",'#FFFFFF');

     $('div[id^="Widget_"]').each(function(){
        $(this).empty();
        $(this).remove();
     });

     map.width(w);
     map.height(h);

     var item = _this.mapContextMenu.menu('getItem', $('#'+_this.mapContextMenu.attr('id')+'_mode')[0]);
     _this.mapState = 'edit';
     _this.mapContextMenu.menu('setText', { target:item.target, text: 'to view mode'});
     _this.toolsPanel.window('open');

     newWin.window('close');
   });
}


MapEditorController.prototype.load_image = function(name, type, checkflag)
{
   var _this = this;

   _this.bgimage=name;
   _this.map.css("background", "url('"+_this.imagepath+"/"+name+"') no-repeat");
   _this.map.css("background-size", "cover");
}


MapEditorController.prototype._context_menu = function(action, w)
{
   var _this = this;

   switch(action)
   {
      case 'gridnone':
         _this.grid = 1;
         break;

      case 'grid5x5':
         _this.grid = 5;
         break;

      case 'grid10x10':
         _this.grid = 10;
         break;

      case 'grid20x20':
         _this.grid = 20;
         break;

      case 'grid50x50':
         _this.grid = 50;
         break;

      case 'new':
         _this.newMap(w);
         break;

      case 'backgroundi':
         var __load_image = _this.load_image.bind(_this);
         _this.ctrlr_fileuploaderchooser.open(_this._toLocalC("choose image ..."), _this._toLocalC("background image")+_this._localDoubleDot(), _this._toLocalC("set"), _this._toLocalC("cancel"), "img", false, true, null, __load_image);
         break;

      case 'load':
         var __load_map = _this.load_map.bind(_this);
         _this.ctrlr_filechooser.open(_this._toLocalC("choose map ..."), _this._toLocalC("load")+_this._localDoubleDot(), _this._toLocalC("load"), _this._toLocalC("cancel"), "map", true, true, _this._toLocalC("file does not exist, new file ?"), __load_map);
         break;

      case 'save':
         _this.saved = {};
         _this.saveTo(_this.saved);
         if(_this.current_file!=false)
            _this.save_map(_this.current_file, "map", false);
         else
         {
            var __save_map = _this.save_map.bind(_this);
            _this.ctrlr_filechooser.open(_this._toLocalC("choose map name ..."), _this._toLocalC("save as")+_this._localDoubleDot(), _this._toLocalC("save"), _this._toLocalC("cancel"), "map", true, false, _this._toLocalC("file exist, overhide it ?"), __save_map);
         }
         break;

      case 'saveas':
         _this.saveTo(_this.saved);
         var __save_map = _this.save_map.bind(_this);
         _this.ctrlr_filechooser.open(_this._toLocalC("choose map name ..."), _this._toLocalC("save as"), _this._toLocalC("save as"), _this._toLocalC("cancel"), "map", true, false, _this._toLocalC("file exist, overhide it ?"), __save_map);
         break;

      case 'delete':
         var __delete_map = _this.delete_map.bind(_this);
         _this.ctrlr_filechooser.open(_this._toLocalC("choose map to delete ..."), _this._toLocalC("delete"), _this._toLocalC("delete"), _this._toLocalC("cancel"), "map", false, false, "", __delete_map);
         break;


      case 'toggle':
         var id=_this.mapContextMenu.attr('id');
         var item = _this.mapContextMenu.menu('getItem', $('#'+id+'_mode')[0]);

         if(_this.mapState == 'edit') {
            _this.mapState = 'view';
            _this.mapContextMenu.menu('setText', { target: item.target, text: 'to edition mode'});
            var _tmp = {};
            _this.toolsPanel.window('close');
            _this.propertiesPanel.window('close');
            _this.saveTo(_tmp);
            _this.loadFrom(_tmp);
            tmp = null;
         }
         else {
            _this.mapState = 'edit';
            _this.mapContextMenu.menu('setText', { target:item.target, text: 'to view mode'});
            _this.toolsPanel.window('open');
            $('div[id^="Widget_"]').each(function(){
               var data = $(this).prop('mea-widgetdata');
               meaWidgetsJar[data[1].value].init(data[0].value, true);
               meaWidgetsJar[data[1].value].disabled(data[0].value, true);
               $(this).draggable('enable');
               $(this).bind('contextmenu', _this.open_widget_menu);
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
   var _this = this;

   if(_this.grid == 1)
      return v;

   var r = parseInt(v/_this.grid) * _this.grid;

   if (Math.abs(v % _this.grid) > (_this.grid / 2)) {
      r += v > 0 ? _this.grid : -_this.grid;
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

   d.top = _this.repair(d.top);
   d.left = _this.repair(d.left);
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
         if(_this.dragDropEntered === true) {
            var id=$(e.data.target).attr('id');
            _this.constrain(e, $("#"+id+"_drag"), $("#"+id+"_model"));
         }
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
         _this.dragDropEntered = false;
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
/*
   var namesvalues_sel = _this.actionPanel.find('[name="namesvalues"] > option');
   namesvalues_sel.each(function() {
      var namevalue = JSON.parse($(this).val());
      data[namevalue.name]=namevalue.value;
   });
*/
   var namesvalues2_sel = $('#actions_win_me_namesvalues2');
   var _data = namesvalues2_sel.datalist('getData')['rows'];
   $.each(_data,function(i,val) {
      data[val['f1'][0]]=val['f1'][1];
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
/*
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
*/
      var namesvalues2_sel = $('#actions_win_me_namesvalues2');
      var data = namesvalues2_sel.datalist('getData')['rows'];
      var found = false;

      $.each(data, function(i,val) {
         if(name == val['f1'][0])
         {
            val['f1'][1]=value;
            val['f2']=name+" = "+value;
            found = true;
            return false;
         }
      });

      if(found === false)
         data.push({f1: [name,value], f2:name+" = "+value});

      console.log(JSON.stringify(data));
      namesvalues2_sel.datalist('loadData',data);
   } 
}


MapEditorController.prototype._xplEditorUp = function()
{
   var _this = this;
   var id = _this.actionPanel.attr('id');
/*
   var sel = _this.actionPanel.find('[name="namesvalues"] option:selected');
   if(sel.val())
   {
      var namevalue = JSON.parse(sel.val());
      $("#"+id+"_names").combobox('setValue',namevalue.name);
      $("#"+id+"_values").combobox('setValue',namevalue.value);
   }
   sel.remove();
*/
   var namesvalues2_sel = $('#'+id+'_namesvalues2');
   var data = namesvalues2_sel.datalist('getData')['rows'];
   var index = namesvalues2_sel.datalist('getRowIndex',namesvalues2_sel.datalist('getSelected'));
   if(index<0)
      return;
   $("#"+id+"_names").combobox('setValue',data[index]['f1'][0]);
   $("#"+id+"_values").combobox('setValue',data[index]['f1'][1]);
   data.splice(index, 1);
   namesvalues2_sel.datalist('loadData',data);
}


MapEditorController.prototype.__aut_listener=function(message)
{
   var _this = this;

   if(_this.mapState=='edit')
      return;

   var data = jQuery.parseJSON(message);

   try {
      $.each(data, function(i,val) {

         $.each(_this.map.find('label[name="'+i+'"]'), function() {
            var _formater = $(this).attr('mea_valueformater');
            if(_formater) {
               var str = '';
               var v = parseFloat(val);
               if (v === false)
                  v = val;
               str = meaFormaters[_formater](v);
               if(str!==false)
                  $(this).text(str);
               else
                  $(this).text(val);
            }
            else {
               $(this).text(val);
            }
         });

         $.each(_this.map.find('input[name="'+i+'"]'), function() {
            var _formater = $(this).attr('mea_valueformater');
            if(_formater) {
               var str = meaFormaters[_formater](val);
               if(str!==false)
                  $(this).val(str);
               else
                  $(this).val(val);
            }
            else {
               $(this).val(val);
            }
         });

         $.each(_this.map.find('div[name="'+i+'"]'), function() {
            var _formater = $(this).attr('mea_valueformater');
            if(_formater)
               meaFormaters[_formater](val, $(this));
            else
               $(this).html(i);
         });
      });
   }
   catch(ex) {
   }
};


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
            var accordion = $('#accordion_'+_this.toolsPanel.attr('id'));
            $.each(meaWidgetsJar, function(i,obj) {
               var p = accordion.accordion('getPanel', obj.getGroup());
               if(!p) {
                  accordion.accordion('add', {
	             title: obj.getGroup(),
	             content: "<div id='grp_"+obj.getGroup()+"'></div>",
	             selected: false
                  });
                  p = accordion.accordion('getPanel', obj.getGroup());
               }
               p.append("<div id='_tip_"+obj.getType()+"'class='widgetIcon'>"+obj.getHtmlIcon()+"</div>");
               $('#widgets_container').append(obj.getHtml());

               if(obj.getTip()!==false) {
                  $('#_tip_'+obj.getType()).tooltip({position: 'right',
                     content: '<span style="color:#fff">'+obj.getTip()+'</span>',
                     onShow: function(){
                        $(this).tooltip('tip').css({
                           backgroundColor: '#666',
                           borderColor: '#666'
                        });
                     }
                  });
               }

               ctrlr_mapEditor.addWidget(obj.getType());

               $.each(obj.getFormaters(), function(i,val) {
                  meaFormaters[i]=val;
               });
            });
            p=accordion.accordion('select',0);
            
         }
         else
            load_widgets(list, i);
      });
   }

   var i=list.length-1;

   load_widgets(list, i);
}

/*
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
*/

jQuery(document).ready(function() {
   var list = [
//      "../widgets/meawidget_lampe.js",
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
