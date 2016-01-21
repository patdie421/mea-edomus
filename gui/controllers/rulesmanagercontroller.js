function RulesManagerController(_zone, _panel, _sources_to_select, _sources_selected, _builded_rules, _button_in, _button_out, _button_up, _button_down, _button_build, _button_apply, _button_rules_delete, _menu)
{
    RulesManagerController.superConstructor.call(this);

   this.current_file=false;

   this.zone = _zone;
   this.panel = _panel;
   this.sources_to_select = _sources_to_select;
   this.sources_selected = _sources_selected; 
   this.builded_rules = _builded_rules;
   this.button_in = _button_in; 
   this.button_out = _button_out; 
   this.button_up = _button_up; 
   this.button_down = _button_down; 
   this.button_build = _button_build; 
   this.button_apply = _button_apply; 
   this.button_rules_delete = _button_rules_delete; 
   this.menu = _menu; 

   this.rules = [];

   this.ctrlr_filechooser = new FileChooserController("#"+this.zone);

   var _RulesManagerController = this;


   $('#'+_RulesManagerController.sources_selected).empty();
   $('#'+_RulesManagerController.builded_rules).empty();

   _RulesManagerController.load_sources(_RulesManagerController.sources_to_select, function() {}, function() {});
   _RulesManagerController.load_builded(_RulesManagerController.builded_rules, function() {}, function() {});

   $('#'+_RulesManagerController.button_in).click(function() {
    return !$('#'+_RulesManagerController.sources_to_select+' option:selected').remove().removeAttr("selected").appendTo('#'+_RulesManagerController.sources_selected);
   });
   $('#'+_RulesManagerController.button_out).click(function() {
    return !$('#'+_RulesManagerController.sources_selected+' option:selected').remove().appendTo('#'+_RulesManagerController.sources_to_select);
      selected.removeAttr("selected");
   });


   $('#'+_RulesManagerController.button_up).click(function() {
      var selected = $('#'+_RulesManagerController.sources_selected+' option:selected');
      var before = selected.prev();
      if (before.length > 0)
         selected.detach().insertBefore(before);
   });
   $('#'+_RulesManagerController.button_down).click(function() {
      var selected = $('#'+_RulesManagerController.sources_selected+' option:selected');
      var next = selected.next();
      if (next.length > 0)
         selected.detach().insertAfter(next);
   });

   $('#'+_RulesManagerController.button_build).click(function() {
      var files=[];
      $('#'+_RulesManagerController.sources_selected+' option').each(function() {
         files.push($(this).text());
      });
      if(files.length<1)
         return;
      _RulesManagerController.build_rules(files); 
   });

   $('#'+_RulesManagerController.button_apply).click(function() {
      var rules_selected = $('#'+_RulesManagerController.builded_rules+' option:selected').text();
      if(rules_selected.length>0)
      {
         _RulesManagerController._apply_rules(rules_selected);
      }
   });

   $('#'+_RulesManagerController.button_rules_delete).click(function() {
      var rules_selected = $('#'+_RulesManagerController.builded_rules+' option:selected').text();
      if(rules_selected.length>0)
      {
         _RulesManagerController.delete_rules(rules_selected, "rules");
          $('#'+_RulesManagerController.builded_rules+' option:selected').removeAttr("selected");
      }
   });
}

extendClass(RulesManagerController, CommonController);


RulesManagerController.prototype.start = function()
{
   var _this = this;

   if (!_this._isAdmin())
   {
   }

   $("#"+_this.panel).height($("#"+_this.zone).height()-35);

   $(document).on( "CenterResize", function( event, arg1, arg2 ) {
      setTimeout( function() {
         $("#"+_this.panel).panel('resize',{
            height: $("#"+_this.zone).height()-35,
            width: $("#"+_this.zone).width() });
      },
      25);
   });

   var evnt = "activatetab_" + _this._toLocalC('rules manager');
   evnt=evnt.replace(/[^a-zA-Z0-9]/g,'_');
   $(document).off(evnt);
   $(document).on(evnt, function( event, tab, arg2 ) {
      $("#"+_this.panel).panel('resize',{
         height: $("#"+_this.zone).height()-35,
         width: $("#"+_this.zone).width() });
      ctrlr_rulesManager.refresh();
   });


   $('#'+_this.menu).show();
}


RulesManagerController.prototype.refresh = function()
{
   var _this = this;

   $('#'+_this.builded_rules).empty();
   _this.load_builded(_this.builded_rules, function() {}, function() {});

   _this.update_select();
}


RulesManagerController.prototype._domenu = function(action)
{
   var _this = this;

   __load_rules_set = _this.load_rules_set.bind(_this);
   __save_rules_set = _this.save_rules_set.bind(_this);
   __delete_rules =   _this.delete_rules.bind(_this);
   __apply_rules =    _this.apply_rules.bind(_this);

   switch(action)
   {
      case 'new':
         _this.current_file=false;
         $('#'+_this.sources_selected).empty();
         _this.load_sources(_this.sources_to_select, function() {}, function() {});
         break;
      case 'save':
         if(_this.current_file!=false)
            _this.save_rules_set(_this.current_file, "rset", false);
         else
            _this.ctrlr_filechooser.open(_this._toLocalC("choose rules set ..."), _this._toLocalC("save as")+_this._localDoubleDot(), _this._toLocalC("save"), _this._toLocalC("cancel"), "rset", true, false, _this._toLocalC("file exist, overhide it ?"), __save_rules_set);
         break;

      case 'load':
         _this.ctrlr_filechooser.open(_this._toLocalC("choose rules set ..."), _this._toLocalC("load")+_this._localDoubleDot(), _this._toLocalC("load"), _this._toLocalC("cancel"), "rset", true, true, _this._toLocalC("file does not exist, new file ?"), __load_rules_set);
         break;

      case 'saveas':
         _this.ctrlr_filechooser.open(_this._toLocalC("choose rules set ..."), _this._toLocalC("save as"), _this._toLocalC("save as"), _this._toLocalC("cancel"), "rset", true, false, _this._toLocalC("file exist, overhide it ?"), __save_rules_set);
         break;

      case 'delete':
         _this.ctrlr_filechooser.open(_this._toLocalC("choose rules set to delete ..."), _this._toLocalC("delete"), _this._toLocalC("delete"), _this._toLocalC("cancel"), "rset", false, false, "", __delete_rules);
         break;

      case 'delete_builded':
         _this.ctrlr_filechooser.open(_this._toLocalC("choose rules to delete ..."), _this._toLocalC("delete"), _this._toLocalC("delete"), _this._toLocalC("cancel"), "rules", false, false, "", __delete_rules);
         break;

      case 'apply':
         _this.ctrlr_filechooser.open(_this._toLocalC("choose rules to apply ..."), _this._toLocalC("apply"), _this._toLocalC("apply"), _this._toLocalC("cancel"), "rules", false, false, "", __apply_rules);
         break;
   }
}


RulesManagerController.prototype.domenu = function(action)
{
   var _this = this;

   _this.__auth(_this, '_domenu', action);
}


RulesManagerController.prototype.save_rules_set = function(name, type, checkflag)
{
   var _this = this;
   var data=[];

   $('#'+_this.sources_selected+' option').each(function() {
      data.push($(this).text());
   });

   $.post("models/put_file.php", { type: type, name: name, file: JSON.stringify(data) }, function(response) {
   if(response.iserror===false)
      _this.current_file=name;
   else
      $.messager.alert(_this._toLocalC('error'), _this._toLocalC("Can't save file")+ " (" + _this._toLocal('server message')+_this._localDoubleDot()+response.errMsg+")", 'error');
   });
}


RulesManagerController.prototype.update_select = function()
{
   var _this = this;
   var _type = "srules";

   $.get("models/get_files_list.php", { type: _type }, function(response) {

      if(response.iserror === false)
      {
         var selected = [];
         $('#'+_this.sources_selected+' option').each(function() {
            $(this).val(-1);
            selected.push($(this))
         });

         $('#'+_this.sources_to_select).empty();
         for(var i in response.values)
         {
            var name = response.values[i].slice(0, -(_type.length+1));
            var found = false;
            for(var j in selected)
            {
               if(selected[j].text() == name)
               {
                  selected[j].val(i);
                  found = true;
                  break;
               }
            }
            if(found===false)
               $('#'+_this.sources_to_select).append(new Option(name, i));
         }

         for(var j in selected)
         {
            if(selected[j].val()==-1)
               selected[j].remove();
         }
      }
   }).done(function() {
   }).fail(function(jqXHR, textStatus, errorThrown) {
      $.messager.show({
         title:_this._toLocalC('error')+_this._localDoubleDot(),
         msg: _this._toLocalC("communication error")+' ('+textStatus+')'
      });
   });
}


RulesManagerController.prototype.load_select = function(_selectid, _type, after_load, on_error)
{
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
   }).done(function() {
   }).fail(function(jqXHR, textStatus, errorThrown) {
      $.messager.show({
         title:_this._toLocalC('error')+_this._localDoubleDot(),
         msg: _this._toLocalC("communication error")+' ('+textStatus+')'
      });
   });
};


RulesManagerController.prototype.load_builded = function(_selectid, after_load, on_error)
{
   var _this = this;

   _this.load_select(_selectid, "rules", after_load, on_error);
}


RulesManagerController.prototype.load_sources = function(_selectid, after_load, on_error)
{
   var _this = this;

   _this.load_select(_selectid, "srules", after_load, on_error);
};


RulesManagerController.prototype.build_rules = function(files)
{
   var _this = this;
   __build_rules = function(name, type, checkflag)
   {
      function basename(file) {
         return file.split('/').reverse()[0];
      }

      $.get("models/build_rules.php", { name: name, files: files }, function(response) {
         if(response.iserror === false)
         {
            _this.load_builded(_this.builded_rules, function() {}, function() {});
         }
         else
         {
            if(response.file && response.line)
            { 
               $.messager.alert(_this._toLocalC('error'),
                                _this._toLocalC('compilation error')+_this._localDoubleDot()+response.message+
                                '.<BR><BR><div align=center>file: '+basename(response.file).slice(0, -7)+"<BR>line: "+response.line+"</div>",
                                'error');
            }
            else
            {
               $.messager.alert(_this._toLocalC('error'),
                                _this._toLocalC('compilation error')+_this._localDoubleDot()+response.errMsg,
                                'error');
            }
         }
      }).done(function() {
      }).fail(function(jqXHR, textStatus, errorThrown) { 
         $.messager.show({
            title:_this._toLocalC('error')+_this._localDoubleDot(),
            msg: _this._toLocalC("communication error")+' ('+textStatus+')'
         });
      });
   }

   _this.ctrlr_filechooser.open(_this._toLocalC("choose builded rules file name ..."),  _this._toLocalC("build")+_this._localDoubleDot(), _this._toLocalC("build"), _this._toLocalC("cancel"), "rules", true, false, _this._toLocalC("file exist, overhide it ?"), __build_rules);
}


RulesManagerController.prototype.apply_rules = function(name, type, checkflag)
{
   _this._apply_rules(name);
}


RulesManagerController.prototype._apply_rules = function(name)
{
   var _this = this;

   $.get("models/apply_rules.php", { name: name }, function(response) {
      if(response.iserror === false)
      {
         $.messager.alert(_this._toLocalC('information'), _this._toLocalC('rules succesfull applied')+'. '+_this._toLocalC('restart automator to commit')+'.', 'info');
      }
      else
      {
         $.messager.alert(_this._toLocalC('error'),
                          _this._toLocalC("can't apply rules")+_this._localDoubleDot()+response.errMsg,
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


RulesManagerController.prototype.delete_rules = function(name, type, checkflag)
{
   this._delete_rules(name, type);
}


RulesManagerController.prototype._delete_rules = function(name, type)
{
   var _this = this;
   var msg="";
   if(type=="rules")
      msg=_this._toLocal("builded rules");
   else
      msg=_this._toLocal("rules set");

   var _delete_rules = function() {
      $.post("models/del_file.php", { name: name, type: type }, function(response) {
 
         if(response.iserror === false)
         {
            _this.load_builded(_this.builded_rules, function() {}, function() {});
         }
         else
         {
            $.messager.alert(_this._toLocalC('error'),
                             _this._toLocalC("can't delete "+msg)+_this._localDoubleDot()+response.errMsg,
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
    $.messager.confirm(_this._toLocalC('deleting')+' "'+msg+'" ...',_this._toLocalC('are you sure you want to delete')+' "'+name+'"'+_this._localDoubleDot(), function(r)
    {
       if(r)
          _delete_rules(name);
    });
}


RulesManagerController.prototype.load_rules_set = function(name, type, checkflag)
{
   var _this = this;

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
               $('#'+_this.sources_to_select+' option').each(function() {
                  if($(this).text()===data[i])
                  {
                     id=$(this).val();
                     $(this).remove();
                     $('#'+_this.sources_selected).append(new Option(data[i], id));
                     found = true;
                     return false;
                  }
               });
               if (found===false)
                  notfound.push(data[i]);
            }
            if(notfound.length != 0)
               $.messager.alert(_this._toLocalC('error'), _this._toLocalC('rule(s) not found')+_this._localDoubleDot()+JSON.stringify(notfound), 'error');
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

   $('#'+_this.sources_selected).empty();
   _this.load_sources(_this.sources_to_select, function() { __load_rules_set(name, type, checkflag); }, function() { alert("error"); } );
};

