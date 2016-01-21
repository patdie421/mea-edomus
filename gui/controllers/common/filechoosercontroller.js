$.extend($.fn.validatebox.defaults.rules, {
   filename_validation: {
      validator: function(value, param){
         return checkRegexp( value, /^[0-9A-Za-z]*$/);
      },
      message: "caractères dans l'intervalle [a-zA-Z0-9] uniquement."
   }
});


function FileChooserController(attachement)
{
   this.attachement = attachement; 
   this.id = "dlg_"+Math.floor((Math.random() * 10000) + 1);
}


extendClass(FileChooserController, CommonController);

FileChooserController.prototype = {
   open: function(_title, _title2, _buttonOKText, _buttonCancelText, _type, _checkflag, _mustexist, _checkmsg, _do)
   {
      var _this = this;
      id=_this.id;
      __do = function(files) 
      {
         name = $('#'+id+"_filename").val();
         if(name==="")
            return -1;
         filename=name+"."+_type;

         if(_checkflag === true)
         { 
            inarray=$.inArray(filename, files);
            if(  (inarray == -1 && _mustexist == true)    // pas dans la liste mais doit exister
              || (inarray != -1 && _mustexist == false) ) // dans la liste mais ne doit pas y etre 
            {
               $.messager.confirm('Confirm', _checkmsg, function(r) {
                  if (r) {
                     _this.close();
                     _do(name, _type, true);
                  }
               });
            }
            else
            {
               _this.close();
               _do(name, _type, false); 
            }
         }
         else
         {
            _this.close();
            _do(name, _type, false);
         }
      };

      $.get("models/get_files_list.php", { type: _type }, function(response) {
         if(response.iserror === false)
         {
            $('#'+id).empty();
            $('#'+id).remove();
            html="<div id='"+id+"' style=\"padding:10px 20px\"> \
               <div id='"+id+"_title' class='ftitle'></div> \
               <form id='"+id+"_fm' method='post' data-options=\"novalidate:false\"> \
                  <div class='fitem'> \
                     <select name='"+id+"_selectfiles' id='"+id+"_selectfiles' size='15' style=\"width:100%;font-family:verdana,helvetica,arial,sans-serif;font-size:12px;\"></select> \
                  </div> \
		  <div class='fitem' style=\"padding-top:10px;\"> \
                     <input id='"+id+"_filename' style=\"width:100%;\"> \
                  </div> \
               </form> \
            </div>";

            $('body').append(html);
            $('#'+id+'_title').text(_title2);
            $('#'+id+'_filename').textbox({ required: true, validType: 'filename_validation'});
            $("#"+id+"_filename").textbox('setValue', "");

            $('#'+id).dialog({
               title: _title,
               width: 350,
               height: 400,
               closed: false,
               cache: false,
               modal: true,
               buttons:[
               {
                  text: _buttonOKText,
                  iconCls:'icon-ok',
                  handler:function() {
                     if(!$('#'+id+'_fm').form('validate'))
                        return -1;
                     __do(response.values);
                  }
               }, {
                  text: _buttonCancelText,
                  iconCls:'icon-cancel',
                  handler:function() {
                     _this.close();
                  }
               }],
               onClose: function() { _this.clean(); },
            });
            $('#'+id+'_selectfiles').empty();
            for(var i in response.values)
               $('#'+id+'_selectfiles').append(new Option(response.values[i].slice(0, -(_type.length+1)), i));

            $('#'+id+'_selectfiles').off('dblclick');
            $('#'+id+'_selectfiles').on('dblclick', function(e) {
               e.preventDefault();
               if(this.value !== false)
               {
                  $('#'+id+"_filename").textbox('setValue', response.values[this.value].slice(0, -(_type.length+1)));
                  __do(response.values);
               }
            });

            $('#'+id+'_selectfiles').off('change');
            $('#'+id+'_selectfiles').on('change', function() {
               if(this.value !== false)
                  $('#'+id+"_filename").textbox('setValue', response.values[this.value].slice(0, -(_type.length+1)));
               else
                  $('#'+id+"_filename").textbox('setValue', "");
            });

            $('#'+id).dialog("open");
         }
      });
   },

   clean: function() {
      console.log("clean");
      $('#'+id).empty();
      $('#'+id).remove();
   },

   close: function() {
      $('#'+id).dialog("close");
   }
};
