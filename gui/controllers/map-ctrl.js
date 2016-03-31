function getValueIndex(data, n) {
   var found = false;

   $.each(data, function(i, val) {
      if(val.name == n) {
         found = i;
         return false;
      }
   });

   if(found !== false)
      return found;
   else
      return false;
}


function MapController(container, map, widgets_container, mapContextMenu)
{
   MapController.superConstructor.call(this);

   this.container =         $('#'+container);
   this.map =               $('#'+map);
   this.mapContextMenu =    $('#'+mapContextMenu);
   this.widgets_container = $('#'+widgets_container);
   this.mapState = 'view';

   this.current_file=false;
   this.imagepath = '/images';
   this.bgimage = false;
   this.bgcolor = false;
   this.current_zindex=0;
   this.objid = 0;

   this.open_context_menu = this._open_context_menu.bind(this);

   this.ctrlr_filechooser = new FileChooserController("#"+container);

   var _this = this;

   _this.map.bind('contextmenu', _this.open_context_menu);
}

extendClass(MapController, CommonController);


MapController.prototype.start = function()
{
   var _this = this;

   return true;
}


MapController.prototype._cleanexit = function()
{
   var _this = this;

   return true;
}


MapController.prototype._open_context_menu = function(e)
{
   var _this = this;

   e.preventDefault();

   _this.mapContextMenu.menu('show', {left: e.pageX, top: e.pageY });

   return false;
}


MapController.prototype._context_menu = function(action, w)
{
   var _this = this;

   switch(action)
   {
      case 'load':
         var _load_map = _this.load_map.bind(_this);
         _this.ctrlr_filechooser.open(_this._toLocalC("choose map ..."), _this._toLocalC("load")+_this._localDoubleDot(), _this._toLocalC("load"), _this._toLocalC("cancel"), "map", true, true, _this._toLocalC("file does not exist, new file ?"), _load_map);
         break;
   }
}


MapController.prototype.createFromWidgetdata = function(obj, d)
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

   p.prop('mea-widgetdata', obj);

   meaWidgetsJar[type].init(id);
   meaWidgetsJar[type].disabled(id, d);

   return p;
}


MapController.prototype.loadFrom = function(s)
{
   var _this = this;

   $('div[id^="Widget_"]').each(function(){
      $(this).empty();
      $(this).remove();
   });

   _this.map.css("background", '');
   _this.map.css("background-size", '');

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

   if(_this.bgimage)
   {
      _this.map.css("background", "url('"+_this.imagepath+"/"+_this.bgimage+"') no-repeat");
      _this.map.css("background-size", "cover");
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

      var l = $('#'+id+" [name]");
      var name = $('#'+id).attr("name");
      if(name)
         l=l.add($('#'+id));
      l.each(function(o) {
         if($(this).attr("mea_notooltip")!='true')
         {
            $(this).tooltip({
               position: 'bottom',
               showDelay: 1000,
               content:"<span style='font-size:8px'>N/A</span>"
            });
         }
      });
   });

   _this.automatorSendAllInputs();
}


MapController.prototype.load_map = function(name, type, checkflag)
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



MapController.prototype.automatorSendAllInputs = function()
{
   var _this = this;

   $.post("CMD/automator.php", { cmnd: "sendallinputs" }, function(response) {
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


MapController.prototype.__aut_listener=function(message)
{
   var _this = this;

   if(_this.mapState=='edit')
      return;

   var data = jQuery.parseJSON(message);
   try {
      $.each(data, function(i,x) {
         var val=x["v"];

         $.each(_this.map.find('[name="'+i+'"]'), function() {
            var v = val;
            var _formater = $(this).attr('mea_valueformater');
            if(_formater) {
               try {
                  var str = meaFormaters[_formater](val, $(this));
                  if(str!==false)
                     v=str;
               }
               catch(e) { console.log( "meaFormater: "+e.message ); }
            }

            if($(this).is('label'))
            {
               $(this).text(v);
            }
            else if($(this).is('input'))
            {
               $(this).val(v);
            }
            else if($(this).is('div'))
            {
            }

            if($(this).attr("mea_notooltip")!='true')
            {
               var t = "N/A";
               if(val !== t)
                  t = x["t"];
               try { $(this).tooltip('update', "<span style='font-size:8px'>"+t+"</span>"); }  catch(e) { console.log("tooltip: "+e.message); };
            }
         });
      });
   }
   catch(ex) {
      console.log(ex.message);
   }
};


MapController.prototype._loadWidgets = function(list, afterLoadCallback)
{
   var _this = this;

   function load_widgets(list, i)
   {
      $.getScript("../widgets/"+list[i], function(data, textStatus, jqxhr) {
         if(jqxhr.status == 200)
            console.log(list[i]+": loaded");
         else
            console.log(list[i]+": error - "+textStatus);
         i=i-1;
         if(i<0)
         {

            $.each(meaWidgetsJar, function(i,obj) {
               obj.setMapsController(_this);
               var style = obj.getStyle();
               if(style)
                  _this.widgets_container.append("<style>"+style+"</style>");
               _this.widgets_container.append(obj.getHtml());
               $.each(obj.getFormaters(), function(i,val) {
                  meaFormaters[i]=val;
               });
            });

            if(afterLoadCallback != false)
               afterLoadCallback();
         }
         else
            load_widgets(list, i);
      }).fail(function( jqxhr, textStatus, exception ) {
         console.log("can't load '"+list[i]+"': "+textStatus);
         i=i-1;
         load_widgets(list, i);
      });
   }

   var i=list.length-1;

   load_widgets(list, i);
}


MapController.prototype.loadWidgets = function(afterLoadCallback)
{
   afterLoadCallback = typeof afterLoadCallback !== 'undefined' ? afterLoadCallback : false;

   var _this = this;

   $.get("models/get_files_list.php", { type: "widget" }, function(response) {
      if(response.iserror === false)
      {
         _this._loadWidgets(response.values, afterLoadCallback);
      }
      else
      {
         $.messager.alert(_this._toLocalC('error'), _this._toLocalC("No widget found")+ " (" + _this._toLocal('server message')+_this._localDoubleDot()+response.errMsg+")", 'error');
      }
   }).done(function() {
   }).fail(function(jqXHR, textStatus, errorThrown) {
      $.messager.show({
         title:_this._toLocalC('error')+_this._localDoubleDot(),
         msg: _this._toLocalC("communication error")+' ('+textStatus+')'
      });
  });
}

