extendClass(MapsSetEditorController, CommonController);

function MapsSetEditorController(_zone, _menu, _panel, _maps_to_select, _maps_selected, _button_in, _button_out, _button_up, _button_down, _button_save, _callback, _toSaveOrLoad)
{
    MapsSetEditorController.superConstructor.call(this);
   this.zone = _zone;
   this.menu = _menu; 
   this.panel = _panel;
   this.maps_to_select = _maps_to_select;
   this.maps_selected = _maps_selected; 
   this.button_in = _button_in; 
   this.button_out = _button_out; 
   this.button_up = _button_up; 
   this.button_down = _button_down; 
   this.button_save = _button_save; 
   this.callback = _callback;
   this.toSaveOrLoad = _toSaveOrLoad;

   this.current_file=false;

   this.maps = [];

   this.ctrlr_filechooser = new FileChooserController("#"+this.zone);

   this.orderedListSelector = new OrderedFilesListSelector(this.maps_to_select, this.maps_selected, this.button_in, this.button_out, this.button_up, this.button_down, "map", this.callback);

   var _MapsSetEditorController = this;


   $('#'+_MapsSetEditorController.button_save).click(function() {
      _MapsSetEditorController.domenu('save');
   });
}


MapsSetEditorController.prototype.start = function()
{
   var _this = this;

   if (!_this._isAdmin())
   {
   }

   $("#"+_this.panel).height($("#"+_this.zone).height()-35);

   $(document).on( "MeaCenterResize", function( event, arg1, arg2 ) {
      setTimeout( function() {
         $("#"+_this.panel).panel('resize',{
            height: $("#"+_this.zone).height()-35,
            width: $("#"+_this.zone).width() });
      },
      25);
   });

   var evnt = "activatetab_" + _this._toLocalC('menu editor');
   evnt=evnt.replace(/[^a-zA-Z0-9]/g,'_');
   $(document).off(evnt);
   $(document).on(evnt, function( event, tab, arg2 ) {
      $("#"+_this.panel).panel('resize',{
         height: $("#"+_this.zone).height()-35,
         width: $("#"+_this.zone).width() });
      _this.orderedListSelector.reload();
   });


   $('#'+_this.menu).show();
}


MapsSetEditorController.prototype.save = function(name, type, checkflag, userdata)
{
   var _this = this;
   var data={};

   data["list"] = _this.orderedListSelector.get();

   $.each(_this.toSaveOrLoad, function(key, value) {
      try {
         var v=_this.toSaveOrLoad[key].get(_this.toSaveOrLoad[key].obj, value, data);
         data[key]=v;
      }
      catch(e) {console.log(e.message);};
   });

 
   $.post("models/put_file.php", { type: type, name: name, file: JSON.stringify(data) }, function(response) {
      if(response.iserror===false)
      {
         if(userdata.getCallback)
            userdata.getCallback(name, data, userdata.userdata);
      }
      else
         $.messager.alert(_this._toLocalC('error'), _this._toLocalC("can't save file")+ " (" + _this._toLocal('server message')+_this._localDoubleDot()+response.errMsg+")", 'error');
   });
}


MapsSetEditorController.prototype.load = function(name, type, checkflag, userdata)
{
   var _this = this;

   if(checkflag === true)
      return;

   function afterLoadCallback(name, data, userdata)
   {
      if(userdata.callback)
         userdata.callback(name, data, userdata.userdata);

      $.each(userdata.data, function(key, value) {
         if(_this.toSaveOrLoad[key])
         {
            try {
               _this.toSaveOrLoad[key].set(_this.toSaveOrLoad[key].obj, value, data); 
            }
            catch(e) {console.log(e.name+": "+e.message);};
         }
      });
   }

   $.get("models/get_file.php", { type: type, name: name }, function(response) {
      if(response.iserror===false)
      {
         var data = JSON.parse(response.file);

         var afterLoadUserData = {
            callback: userdata.setCallback,
            userdata: userdata.callbackData,
            data: data
         }
         _this.orderedListSelector.set(data.list, afterLoadCallback, afterLoadUserData);
      }
   }).done(function() {
   }).fail(function(jqXHR, textStatus, errorThrown) {
      $.messager.show({
         title:_this._toLocalC('error')+_this._localDoubleDot(),
         msg: _this._toLocalC("communication error")+' ('+textStatus+')'
      });
   });
}


MapsSetEditorController.prototype._domenu = function(action)
{
   var _this = this;
   __load_menu = _this.load.bind(_this);
   __save_menu = _this.save.bind(_this);
   __del_menu = _this.orderedListSelector.del.bind(_this.orderedListSelector);

   switch(action)
   {
      case 'new':
         _this.current_file=false;
          $('#'+_this.maps_selected).datalist({data:[]});
          $.each(_this.toSaveOrLoad, function(key, value) {
             {
                try {
                  _this.toSaveOrLoad[key].set(_this.toSaveOrLoad[key].obj, "", false);
                }
                catch(e) {console.log(e.name+": "+e.message);};
             }
         });
         _this.orderedListSelector._getFilesList();

         break;
      case 'save':
         if(_this.current_file!=false)
            __save_menu(_this.current_file, "menu", false);
         else
            _this.ctrlr_filechooser.open(_this._toLocalC("choose maps set name ..."), _this._toLocalC("save as")+_this._localDoubleDot(), _this._toLocalC("save"), _this._toLocalC("cancel"), "menu", true, false, _this._toLocalC("file exist, overhide it ?"), __save_menu, { getCallback: function(name, selected, userdata) { _this.current_file = name; }, callbackData: ""});
         break;

      case 'load':
         _this.ctrlr_filechooser.open(_this._toLocalC("choose maps set ..."), _this._toLocalC("load")+_this._localDoubleDot(), _this._toLocalC("load"), _this._toLocalC("cancel"), "menu", true, true, _this._toLocalC("file does not exist, new file ?"), __load_menu, { setCallback: function(name, selected, userdata) { _this.current_file = name; }, callbackData: "" });
         break;

      case 'saveas':
         _this.ctrlr_filechooser.open(_this._toLocalC("choose maps set name ..."), _this._toLocalC("save as"), _this._toLocalC("save as"), _this._toLocalC("cancel"), "menu", true, false, _this._toLocalC("file exist, overhide it ?"), __save_menu, { getCallback: function(name, selected, userdata) { _this.current_file = name; }, callbackData: ""});
         break;

      case 'delete':
         _this.ctrlr_filechooser.open(_this._toLocalC("choose map set to delete ..."), _this._toLocalC("delete"), _this._toLocalC("delete"), _this._toLocalC("cancel"), "menu", false, false, "", __del_menu);
         break;
   }
}


MapsSetEditorController.prototype.domenu = function(action)
{
   var _this = this;

   _this.__auth(_this, '_domenu', action);
}
