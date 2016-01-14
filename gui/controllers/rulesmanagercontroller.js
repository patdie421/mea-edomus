function RulesManagerController(_files_to_sel, _files_sel, _button_in, _button_out, _button_up, _button_down, _menu)
{
   this.current_file=false;

   this.files_to_sel = _files_to_sel;
   this.files_sel = _files_sel; 
   this.button_in = _button_in; 
   this.button_out = _button_out; 
   this.button_up = _button_up; 
   this.button_down = _button_down; 
   this.menu = _menu; 

   this.ctrlr_filechooser = new FileChooserController();

   _this = this;

   $('#'+_this.files_sel).empty();
   _this.load_select(_this.files_to_sel, function() {}, function() {});


   $('#'+_this.button_in).click(function() {
    return !$('#'+_this.files_to_sel+' option:selected').remove().removeAttr("selected").appendTo('#'+_this.files_sel);
   });
   $('#'+_this.button_out).click(function() {
    return !$('#'+_this.files_sel+' option:selected').remove().appendTo('#'+_this.files_to_sel);
      selected.removeAttr("selected");
   });


   $('#'+_this.button_up).click(function() {
      var selected = $('#'+_this.files_sel+' option:selected');
      var before = selected.prev();
      if (before.length > 0)
         selected.detach().insertBefore(before);
   });
   $('#'+this.button_down).click(function() {
      var selected = $('#'+_this.files_sel+' option:selected');
      var next = selected.next();
      if (next.length > 0)
         selected.detach().insertAfter(next);
   });

   $('#'+_this.menu).show();
}


extendClass(RulesManagerController, CommonController);


RulesManagerController.prototype.domenu = function(action)
{
   _this = this;

   __load_rules_set = _this.load_rules_set.bind(ctrlr_rulesManager);
   __save_rules_set = _this.save_rules_set.bind(ctrlr_rulesManager);

   switch(action)
   {
      case 'new':
         _this.current_file=false;
         $('#'+_this.files_sel).empty();
         _this.load_select(_this.files_to_sel, function() {}, function() {});
         break;
      case 'save':
         if(_this.current_file!=false)
            _this.save_rules_set(_this.current_file, "rset", false);
         else
            _this.ctrlr_filechooser.open(_this._toLocalC("Choose rules set ..."),  _this._toLocalC("save as")+_this._localDoubleDot(), _this._toLocalC("save"), _this._toLocalC("cancel"), "rset", true, true, _this._toLocalC("file exist, overhide it ?"), __save_rules_set);
         break;

      case 'load':
         _this.ctrlr_filechooser.open(_this._toLocalC("Choose rules set ..."), _this._toLocalC("load")+_this._localDoubleDot(), _this._toLocalC("load"), _this._toLocalC("cancel"), "rset", true, true, _this._toLocalC("file does not exist, new file ?"), __load_rules_set);
         break;

      case 'saveas':
         _this.ctrlr_filechooser.open(_this._toLocalC("Choose rules set ..."), _this._toLocalC("save as"), _this._toLocalC("save as"), _this._toLocalC("cancel"), "rset", true, false, _this._toLocalC("file exist, overhide it ?"), __save_rules_set);
         break;
   }
}


RulesManagerController.prototype.save_rules_set = function(name, type, checkflag)
{
   _this = this;

   var data=[];

   $('#'+_this.files_sel+' option').each(function() {
      data.push($(this).text());
   });

   $.post("models/put_file.php", { type: type, name: name, file: JSON.stringify(data) }, function(response) {
   if(response.iserror===false)
      _this.current_file=name;
   else
      $.messager.alert('Error', _this._toLocalC("Can't save file")+ " (" + _this._toLocal('server message')+_this._localDoubleDot()+response.errMsg+")", 'error');
   });
}


RulesManagerController.prototype.load_select = function(_selectid, after_load, on_error)
{
   _type="srules";

   $.get("models/get_files_list.php", { type: _type }, function(response) {
      $('#'+_selectid).empty();
      if(response.iserror === false)
      {
         $('#'+_selectid).empty();
         for(var i in response.values)
            $('#'+_selectid).append(new Option(response.values[i].slice(0, -(_type.length+1)), i));
         after_load();
      }
      else
      {
         on_error();
      }
   });
};


RulesManagerController.prototype.load_rules_set = function(name, type, checkflag)
{
   _this = this;

   __load_rules_set = function(name, type, checkflag)
   {
      _this.current_file=name;
      if(checkflag === true)
         return;

      $.get("models/get_file.php", { type: type, name: name }, function(response) {
         if(response.iserror===false)
         {
            var data = JSON.parse(response.file);
            var notfound = [];
            for (var i in data)
            {
               var found = false;
               $('#'+_this.files_to_sel+' option').each(function() {
                  if($(this).text()===data[i])
                  {
                     id=$(this).val();
                     $(this).remove();
                     $('#'+_this.files_sel).append(new Option(data[i], id));
                     found = true;
                     return false;
                  }
               });
               if (found===false)
                  notfound.push(data[i]);
            }
            if(notfound.length != 0)
               $.messager.alert('Error', _this._toLocalC('rule(s) not found')+_this._localDoubleDot()+JSON.stringify(notfound), 'error');
         }
         else
         {
         }
      });
   };

   $('#'+_this.files_sel).empty();
   _this.load_select(_this.files_to_sel, function() { __load_rules_set(name, type, checkflag); }, function() { alert("error"); } );
};
