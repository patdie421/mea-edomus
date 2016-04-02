extendClass(NavigationPanelController, CommonController);

function NavigationPanelController(ctrlr_map)
{
   this.mapsSetdata = false;
   this.current_map = false;

   this.old_x = -1;
   this.old_y = -1;
   this.timer = false;
   this.on = false;

   this.ctrlr_map = ctrlr_map;
}


NavigationPanelController.prototype.getMapIndex = function(map)
{
   var _this = this;

   var found = false;
   $.each(_this.mapsSetData.list, function(i, value) {
      if(value==map)
      {
         found = i;
         return false;
      }
   });
   return found;
}


NavigationPanelController.prototype.getMapPrevIndex = function(map)
{
   var _this = this;
   var i=_this.getMapIndex(map);
   if(i===false)
      return false;

   if(i===0)
      i=_this.mapsSetData.list.length-1;
   else
      --i;

   return i;
}


NavigationPanelController.prototype.getMapNextIndex = function(map)
{
   var _this = this;
   var i=_this.getMapIndex(map);
   if(i===false)
      return false;

   if(i===_this.mapsSetData.list.length-1)
      i=0;
   else
      ++i;

   return i;
}


NavigationPanelController.prototype.set_mapselection = function(map)
{
   var _this = this;
   var v=_this.getMapIndex(map);
   if(v!==false)
      $("#mapselection").combobox('setValue', v);
}


NavigationPanelController.prototype.redraw = function()
{

   var np=$("#navpanel");
   var x=np.offset().left;
   var y=np.offset().top;

   var _top=y;
   var _left=x;

   if(y >  window.innerHeight)
      var _top = window.innerHeight - 50 - 50;
   if(x >  window.innerWidth)
      var _left = window.innerWidth - 600 - 50;

   np.offset({ top: _top, left: _left });   
}


NavigationPanelController.prototype.initNavigationBar = function(mapSetName, done, error, userdata)
{
   var _this = this;

   var html =
   "<div style='width: 100%; display: table;'>" +
      "<div style='display: table-row'>" +
         "<div style='display: table-cell; height:100%; width: 34px; text-align: center; vertical-align: middle;'>" +
            "<a id='bleft' href='javascript:void(0)' style='width:30px;height:30px'></a>" +
         "</div>" +
         "<div style='display: table-cell; height:100%; width:86px; text-align: center; vertical-align: middle;'>" +
            "<a id='shortcut1' href='javascript:void(0)' style='width:82px;'>MAP1</a>" +
         "</div>" +
         "<div style='display: table-cell; height:100%; width:86px; text-align: center; vertical-align: middle;'>" +
            "<a id='shortcut2' href='javascript:void(0)' style='width:82px;'>MAP2</a>" +
         "</div>" +
         "<div style='display: table-cell; height:100%; width:86px; text-align: center; vertical-align: middle;'>" +
            "<a id='shortcut3' href='javascript:void(0)' style='width:82px;'>MAP3</a>" +
         "</div>" +
         "<div style='display: table-cell; height:100%; width:86px; text-align: center; vertical-align: middle;'>" +
            "<a id='shortcut4' href='javascript:void(0)' style='width:82px;'>MAP4</a>" +
         "</div>" +
         "<div style='display: table-cell; height:100%; text-align: center; vertical-align: middle;'>" +
            "<input id=mapselection style='width:150px'>" +
         "</div>" +
         "<div style='display: table-cell; height:100%; width: 34px; text-align: center; vertical-align: middle;'>" +
            "<a id='bright' href='javascript:void(0)' style='width:30px;height:30px'></a>" +
         "</div>" +
      "</div>" +
   "</div>";

   var _top = window.innerHeight - 50 - 50;
   var _left = window.innerWidth - 600 - 50;

   $('#navpanel').remove();

   $('body').append("<div id='navpanel' class='navpanel' style='display:hidden; top:"+_top+"px; left:"+_left+"px;'>"+html+"</div>");
   $('#navpanel').draggable({ });

   $("#bleft").linkbutton({
      iconCls: 'icon-mealeftarrow',
   }).bind('click', function() {
      var i=_this.getMapPrevIndex(_this.current_map);
      if(i!==false)
      {
         try {
            _this.ctrlr_map.load_map(_this.mapsSetData.list[i],"map",false);
            _this.current_map = _this.mapsSetData.list[i];
            _this.set_mapselection(_this.mapsSetData.list[i])
         }
         catch(e) {console.log(e.message);};
      }
   });

   $("#bright").linkbutton({
      iconCls: 'icon-mearightarrow',
   }).bind('click', function() {
      var i=_this.getMapNextIndex(_this.current_map);
      if(i!==false)
      {
         try {
            _this.ctrlr_map.load_map(_this.mapsSetData.list[i],"map",false);
            _this.current_map = _this.mapsSetData.list[i];
            _this.set_mapselection(_this.mapsSetData.list[i])
         }
         catch(e) {console.log(e.message);};
      }
   });

   $("#shortcut1").linkbutton({
   });

   $("#shortcut2").linkbutton({
   });

   $("#shortcut3").linkbutton({
   });

   $("#shortcut4").linkbutton({
   });

   $("#mapselection").combobox({
   });

   var type="menu";
   $.get("models/get_file.php", { name: mapSetName, type: type }, function(response) {
      if(response.iserror === false)
      {
         var combolist = [];
         _this.mapsSetData = JSON.parse(response.file);

         for(var i in _this.mapsSetData.list)
         {
            combolist.push({f1: i, f2:_this.mapsSetData.list[i]});
         }
         $("#mapselection").combobox({
            valueField:'f1',
            textField:'f2',
            data: combolist,
            onSelect: function(record) 
            {
               try {
                  _this.ctrlr_map.load_map(record.f2,"map",false);
                  _this.current_map = record.f2;
                  _this.set_mapselection(_this.current_map);
               } catch(e) { console.log(e.message); }
            }
         });

         $.each(["1","2","3","4"], function(i, value) {
            var sc = "shortcut"+value;
            if(_this.mapsSetData[sc])
            {
               $("#"+sc).linkbutton({text: _this.mapsSetData[sc], disabled: false});
               $("#"+sc).bind('click', function() {
                  try {
                     _this.ctrlr_map.load_map(_this.mapsSetData[sc],"map",false);
                     _this.current_map = _this.mapsSetData[sc];
                     _this.set_mapselection(_this.current_map);
                  } catch(e) { console.log(e.message); }
               });
            }
            else
            {
               $("#"+sc).linkbutton({text: "", disabled: true});
            } 
         });

         if(done)
            done();
      }
      else
      {
      }
   }).done(function() {
   }).fail(function(jqXHR, textStatus, errorThrown) {
      console.log(textStatus);
      if(error)
         error()

      $.messager.show({
         title:_this._toLocalC('error')+_this._localDoubleDot(),
         msg: _this._toLocalC("communication error")+' ('+textStatus+')'
      });
   });
}


NavigationPanelController.prototype.init = function(mapsSet_name, type, checkflag, map_name)
{
   var _this = this;

   _this.initNavigationBar(mapsSet_name,
      function() {
         if(_this.mapsSetData["list"].length)
         {
            _this.current_map = false;
            if(map_name !== false)
            {
               if(_this.getMapNextIndex(map_name)!==false)
                  _this.current_map = map_name;
            }
            if(_this.current_map === false)
            {
               if(_this.mapsSetData["defaultmap"])
                  _this.current_map = _this.mapsSetData["defaultmap"];
               else
                  _this.current_map = _this.mapsSetData["list"][0];
            }

            _this.ctrlr_map.load_map(_this.current_map, "map", false);
            _this.set_mapselection(_this.current_map, _this.mapsSetData);
         }
      },
      function(userdata) {
         console.log("init error");
      },
      false);

   $('body').bind('mousemove click', function(e) {
      var x = e.pageX;
      var y = e.pageY;
      if(x != _this.old_x || y != _this.old_y || e.type == "click")
      {
         if(_this.on === false)
         {
            $('#navpanel').fadeIn('fast');
            $('body').css({ cursor: 'auto' });
            _this.on=true;
         }

         if(_this.timer !== false)
            clearTimeout(_this.timer);

         _this.timer=setTimeout(function() {
            $("#mapselection").combobox('hidePanel');
            $('#navpanel').fadeOut('slow');
            $('body').css({ cursor: 'none' });
            _this.on=false;
         }, 5000);

         _this.old_x = x;
         _this.old_y = y;
      }
   });
}
