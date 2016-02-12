function MapController(container, map, mapContextMenu, widgets_container)
{
   MapController.superConstructor.call(this);

   this.container =         $('#'+container);
   this.map =               $('#'+map);
   this.mapContextMenu =    $('#'+mapContextMenu);
   this.widgets_container = $('#'+widgets_container);

   this.open_context_menu = this._open_context_menu.bind(this);

   this.ctrlr_filechooser = new FileChooserController("#"+container);

   this.imagepath = '/images';

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

   if(d === true)
      p.bind('contextmenu', _this.open_widget_menu);

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
   else
   {
      _this.map.css("background", '');
      _this.map.css("background-size", '');
   }

   $.each(s['widgets'], function(i, obj) {
      console.log("widget: "+i);
      _this.createFromWidgetdata(obj, false);
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


MapController.prototype._context_menu = function(action, w)
{
   var _this = this;

   switch(action)
   {
      case 'load':
         var __load_map = _this.load_map.bind(_this);
         _this.ctrlr_filechooser.open(_this._toLocalC("choose map ..."), _this._toLocalC("load")+_this._localDoubleDot(), _this._toLocalC("load"), _this._toLocalC("cancel"), "map", true, true, _this._toLocalC("file does not exist, new file ?"), __load_map);
         break;
   }
}


MapController.prototype.__aut_listener=function(message)
{
   var _this = this;

//   if(_this.mapState=='edit')
//      return;

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


MapController.prototype.loadWidgets = function(list, callback)
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
//               $('#widgets_container').append(obj.getHtml());
               _this.widgets_container.append(obj.getHtml());

               $.each(obj.getFormaters(), function(i,val) {
                  meaFormaters[i]=val;
               });
               
            });
            callback();
            
         }
         else
            load_widgets(list, i);
      });
   }

   var i=list.length-1;

   load_widgets(list, i);
}
