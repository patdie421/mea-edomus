function RulesEditorController(_editorzone, _editordiv, _menu, _contextmenu)
{
   RulesEditorController.superConstructor.call(this);

   this.current_file=false;

   this.editorzone = _editorzone;
   this.editordiv = _editordiv;
   this.menu = _menu;
   this.contextmenu = _contextmenu;

   this.editor = ace.edit(this.editordiv);
   this.editor.setOption("fixedWidthGutter", true);
   this.editor.setTheme("ace/theme/xcode");
   this.editor.session.setMode("ace/mode/mearules");
   this.editor.session.setTabSize(3);
   this.editor.session.setUseSoftTabs(true);
   this.editor.setOptions({ fontFamily: "'Lucida Console', Monaco, monospace" });

   this.ctrlr_filechooser = new FileChooserController("#"+this.editorzone);
}

extendClass(RulesEditorController, CommonController);


RulesEditorController.prototype.start = function()
{
   var _this = this;

   if (!_this._isAdmin())
   {
   }

   var evnt = "activatetab_" + translationController.toLocalC('rules editor');
   evnt=evnt.replace(/[^a-zA-Z0-9]/g,'_');
   $(document).off(evnt);
   $(document).on(evnt, function( event, tab, arg2 ) {
      $("#"+_this.editordiv).height($("#"+_this.editorzone).height()-35);
   });
   $("#"+_this.editordiv).height($("#"+_this.editorzone).height()-35);

   $(document).on( "CenterResize", function( event, arg1, arg2 ) {
      setTimeout( function() {
         $("#"+_this.editordiv).height($("#"+_this.editorzone).height()-35);
         _this.editor.resize();
      },
      25);
   });

   _this.editor.setValue("", -1);
   _this.editor.resize();

/*
   _this.editor.container.addEventListener("contextmenu", function(e) {
       _this.open_context_menu(e);
   });
*/
   $("#"+_this.menu).show();
   $("#"+_this.editordiv).show();
};


RulesEditorController.prototype._open_context_menu = function(x, y)
{
   var _this = this;

   $("#"+_this.contextMenu).menu('show', {left: x, top: y });
}


RulesEditorController.prototype.open_context_menu = function(e)
{
   var _this = this;

   e.preventDefault();

   _this._open_context_menu(e.pageX, e.pageY);

   return false;
}


RulesEditorController.prototype.open = function(name, type, checkflag)
{
   var _this = this;

   _this.current_file=name;
   if(checkflag === true)
      return;

   $.get("models/get_file.php", { type: type, name: name }, function(response) {
      if(response.iserror===false)
      {
         _this.editor.setValue(response.file, -1);
      }
      else
      {
         $.messager.alert(_this._toLocalC('error'), _this._toLocalC("can't load file")+ " (" + _this._toLocal('server message')+_this._localDoubleDot()+response.errMsg+")", 'error');
      }
   }).done(function() {
   }).fail(function(jqXHR, textStatus, errorThrown) {
      $.messager.show({
         title:_this._toLocalC('error')+_this._localDoubleDot(),
         msg: _this._toLocalC("communication error")+' ('+textStatus+')'
      });
   });
};


RulesEditorController.prototype.saveas = function(name, type, checkflag)
{
   var _this = this;

   _this.current_file=name;

   var data = _this.editor.getValue();

   $.post("models/put_file.php", { type: type, name: name, file: data }, function(response) {
      if(response.iserror===false)
         _this.current_file=name;
      else
      {
         $.messager.alert(_this._toLocalC('error'), _this._toLocalC("can't save file")+ " (" + _this._toLocal('server message')+_this._localDoubleDot()+response.errMsg+")", 'error');
      }
   }).done(function() {
   }).fail(function(jqXHR, textStatus, errorThrown) {
      $.messager.show({
         title:_this._toLocalC('error')+_this._localDoubleDot(),
         msg: _this._toLocalC("communication error")+' ('+textStatus+')'
      });
   });
};


RulesEditorController.prototype.delete = function(name, type, checkflag)
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
                             _this._toLocalC("can't delete rules source file")+_this._localDoubleDot()+response.errMsg,
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
   $.messager.confirm(_this._toLocalC('deleting')+' '+_this._toLocal('rules source file ...'),_this._toLocalC('are you sure you want to delete')+' "'+name+'"'+_this._localDoubleDot(), function(r)
   {
      if(r)
         _delete_rules(name);
   });
}


RulesEditorController.prototype.domenu = function(action)
{
   var _this = this;

   var __open_srules = _this.open.bind(_this);
   var __saveas_srules = _this.saveas.bind(_this);
   var __delete_srules = _this.delete.bind(_this);

   switch(action)
   {
      case 'new':
         _this.current_file=false;
         _this.editor.setValue("");
         break;

      case 'open':
         _this.ctrlr_filechooser.open(_this._toLocalC("choose rules sources ..."), _this._toLocalC("open")+_this._localDoubleDot(), _this._toLocalC("open"), _this._toLocalC("cancel"), "srules", true, true, _this._toLocalC("file does not exist, new file ?"), __open_srules);
         break;

      case 'saveas':
         _this.ctrlr_filechooser.open(_this._toLocalC("choose rules sources ..."), _this._toLocalC("save as")+_this._localDoubleDot(), _this._toLocalC("save as"), _this._toLocalC("cancel"), "srules", true, false, _this._toLocalC("file exist, overhide it ?"), __saveas_srules);
         break;

      case 'save':
         if(_this.current_file!=false)
            __saveas_srules(_this.current_file,"srules",false);
         else
            _this.ctrlr_filechooser.open(_this._toLocalC("choose rules sources ..."), _this._toLocalC("save as")+_this._localDoubleDot(), _this._toLocalC("save as"), _this._toLocalC("cancel"), "srules", true, false, _this._toLocalC("file exist, overhide it ?"), __saveas_srules);
         break;

      case 'delete':
            _this.ctrlr_filechooser.open(_this._toLocalC("choose rules sources ..."), _this._toLocalC("delete")+_this._localDoubleDot(), _this._toLocalC("delete"), _this._toLocalC("cancel"), "srules", false, false, "", __delete_srules);
         break;
      default:
         break;
   }
}
