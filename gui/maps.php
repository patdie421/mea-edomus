<?php
include_once('lib/configs.php');
include_once('lib/php/translation.php');
include_once('lib/php/$LANG/translation.php');
mea_loadTranslationData($LANG,'');
session_start();
?>

<!DOCTYPE html>
<html style="width:100px;margin:0">
<head>
   <title>
   <?php echo $TITRE_APPLICATION;?>
   </title>
   <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
   <meta name="viewport" content="width=device-width, initial-scale=0.99">
   <meta name="description" content="domotique DIY !">

   <link rel="stylesheet" type="text/css" href="lib/jquery-easyui-1.4.4/themes/default/easyui.css">
   <link rel="stylesheet" type="text/css" href="lib/jquery-easyui-1.4.4/themes/icon.css">
   <link rel="stylesheet" type="text/css" href="lib/jquery-easyui-1.4.4/themes/color.css">
   <link rel="stylesheet" type="text/css" href="lib/mea-edomus.css">

   <script type="text/javascript" src="lib/jquery-easyui-1.4.4/jquery.min.js"></script>
   <script type="text/javascript" src="lib/jquery-easyui-1.4.4/jquery.easyui.min.js"></script>
   <script type="text/javascript" src="lib/noty-2.2.10/js/noty/packaged/jquery.noty.packaged.min.js"></script>

   <script type="text/javascript" src="lib/highstock-4.2.3/js/highstock.js"></script>
   <script type="text/javascript" src="lib/highcharts-4.2.3/js/highcharts-more.js"></script>
   <script type="text/javascript" src="lib/highcharts-4.2.3/js/modules/solid-gauge.js"></script>
   <script type="text/javascript" src="lib/highcharts-4.2.3/js/modules/exporting.js"></script>
   <script type="text/javascript" src="lib/highcharts-4.2.3/js/modules/offline-exporting.js"></script>
   <script type="text/javascript" src="lib/highcharts-4.2.3/js/modules/no-data-to-display.js"></script>
   <script type="text/javascript" src="lib/highcharts-4.2.3/js/themes/grid-light.js"></script>
</head>

<script type="text/javascript" src="models/common/models-utils.js"></script>

<script type="text/javascript" src="controllers/common/meaobject.js"></script>
<script type="text/javascript" src="controllers/common/commoncontroller.js"></script>
<script type="text/javascript" src="controllers/common/translationcontroller.js"></script>
<script type="text/javascript" src="controllers/common/credentialcontroller.js"></script>
<script type="text/javascript" src="controllers/common/livecomcontroller.js"></script>
<script type="text/javascript" src="controllers/common/filechoosercontroller.js"></script>

<script type="text/javascript" src="widgets/meawidget.js"></script>

<!-- surcharge des méthodes du controleur de traduction spécifiques à une langue donnée -->
<?php
   echo "<script type='text/javascript' src='lib/js/".$LANG."/mea-translation.js'></script>"; // extension
?>

<script type="text/javascript" src="controllers/map-ctrl.js"></script>

<script type="text/javascript">

function toAdmin()
{
   window.location = "login.php";
}


function overflow(s)
{
   if(s=='hidden')
   {
      $("#container_mp").removeClass('mp_overflow_scroll');
      $("#container_mp").addClass('mp_overflow_hidden');
   }
   else
   {
      $("#container_mp").removeClass('mp_overflow_hidden');
      $("#container_mp").addClass('mp_overflow_scroll');
   }
}

var menu_data = false;
var current_map = false;


function getMapIndex(map, menu_data)
{
   var found = false;
   $.each(menu_data.list, function(i, value) {
      if(value==map)
      {
         found = i;
         return false;
      }
   });
   return found;
}


function getMapPrevIndex(map, menu_data)
{
   var i=getMapIndex(map, menu_data);
   if(i===false)
      return false;

   if(i===0)
      i=menu_data.list.length-1;
   else
      --i;

   return i;
}


function getMapNextIndex(map, menu_data)
{
   var i=getMapIndex(map, menu_data);
   if(i===false)
      return false;

   if(i===menu_data.list.length-1)
      i=0;
   else
      ++i;

   return i;
}


function set_mapselection(map, menu_data)
{
   var v=getMapIndex(map, menu_data);
   if(v!==false)
      $("#mapselection").combobox('setValue', v);
}


function init_menu(menudef, done, error, userdata)
{
   $("#bleft").linkbutton({
      iconCls: 'icon-mealeftarrow',
   }).bind('click', function() {
      var i=getMapPrevIndex(current_map, menu_data);
      if(i!==false)
      {
         try {
            ctrlr_map.load_map(menu_data.list[i],"map",false);
            current_map = menu_data.list[i];
            set_mapselection(menu_data.list[i], menu_data)
         }
         catch(e) {console.log(e.message);};
      }
   });

   $("#bright").linkbutton({
      iconCls: 'icon-mearightarrow',
   }).bind('click', function() {
      var i=getMapNextIndex(current_map, menu_data);
      if(i!==false)
      {
         try {
            ctrlr_map.load_map(menu_data.list[i],"map",false);
            current_map = menu_data.list[i];
            set_mapselection(menu_data.list[i], menu_data)
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
   $.get("models/get_file.php", { name: menudef, type: type }, function(response) {
      if(response.iserror === false)
      {
         var combolist = [];
         menu_data = JSON.parse(response.file);

         for(var i in menu_data.list)
         {
            combolist.push({f1: i, f2:menu_data.list[i]});
         }
         $("#mapselection").combobox({
            valueField:'f1',
            textField:'f2',
            data: combolist,
            onSelect: function(record) 
            {
               try {
                  ctrlr_map.load_map(record.f2,"map",false);
                  current_map = record.f2;
                  set_mapselection(current_map, menu_data);
               } catch(e) { console.log(e.message); }
            }
         });

         $.each(["1","2","3","4"], function(i, value) {
            var sc = "shortcut"+value;
            if(menu_data[sc])
            {
               $("#"+sc).linkbutton({text: menu_data[sc], disabled: false});
               $("#"+sc).bind('click', function() {
                  try {
                     ctrlr_map.load_map(menu_data[sc],"map",false);
                     current_map = menu_data[sc];
                     set_mapselection(current_map, menu_data);
                  } catch(e) { console.log(e.message); }
               });
            }
            else
            {
               $("#"+sc).linkbutton({text: "", disabled: true});
            } 
         });

         if(done)
            done(menu_data, userdata);
      }
      else
      {
      }
   }).done(function() {
   }).fail(function(jqXHR, textStatus, errorThrown) {
      console.log(textStatus);
      if(error)
         error(userdata)
/*
      $.messager.show({
         title:_this._toLocalC('error')+_this._localDoubleDot(),
         msg: _this._toLocalC("communication error")+' ('+textStatus+')'
      });
*/
   });
}


jQuery(document).ready(function(){
   $.ajaxSetup({ cache: false });

<?php
   $IP=getenv('REMOTE_ADDR');
   echo "LANG='$LANG';";

   if(isset($_REQUEST['view'])) {
      echo "var destview=\""; echo $_REQUEST['view']; echo "\";\n";
   }
   else {
      echo "var destview=\"\";\n";
   }
   echo "var socketio_port="; echo $IOSOCKET_PORT; echo ";\n";
?>

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

   //
   // initialisation des controleurs
   //
   // pour la traduction
   translationController = new TranslationController();
   translationController.loadDictionaryFromJSON("lib/translation_"+LANG+".json");
   extend_translation(translationController);

   // controleur d'habilitation
   credentialController = new CredentialController("models/get_auth.php");

   // initialisation et lancement des communications "live"
   liveComController = new LiveComController(socketio_port);

   // lancement du controleur de communication live (si possible)
   liveComController.start(
   function(s){
      ctrlr_map = new MapController(
         "container_mp",
         "map_mp",
         "widgets_container_mp",
         "map_cm_mp");
      ctrlr_map.linkToTranslationController(translationController);
      ctrlr_map.linkToCredentialController(credentialController);

      ctrlr_map.start();

      if(s!=null) {
         var aut_listener=ctrlr_map.__aut_listener.bind(ctrlr_map);
          s.on('aut', aut_listener);
      }
      else {
      }

      var filechooser = new FileChooserController("#container_mp");
 
      var old_x = -1, old_y = -1;
      var timer = false;
      var on = false;

      ctrlr_map.loadWidgets(function() {
         filechooser.open(ctrlr_map._toLocalC("choose maps set ..."),
            ctrlr_map._toLocalC("open"),
            ctrlr_map._toLocalC("open"),
            ctrlr_map._toLocalC("cancel"),
            "menu",
            false,
            false,
            "",
            function(name, type, checkflag) {
               $('body').bind('mousemove click', function(e) {
                  var x = e.pageX;
                  var y = e.pageY;
                  if(x != old_x || y != old_y || e.type == "click")
                  {
                     if(on === false)
                     {
                        $('#dd').fadeIn('fast');
                        $('html').css({ cursor: '' });
                        on=true;
                     }

                     if(timer !== false)
                        clearTimeout(timer);
                        timer=setTimeout(function() {
                           $("#mapselection").combobox('hidePanel');
                           $('#dd').fadeOut('slow');
                           $('html').css({ cursor: 'none' });
                           on=false;
                        }, 5000);

                     old_x = x;
                     old_y = y;
                  }
               });
               $('body').append("<div id='dd' class='navpanel' style='display:hidden'>"+html+"</div>");
               $('#dd').draggable({ });
            
               init_menu(name,
                         function(maps_set, userdata) {
                            if(maps_set["list"].length)
                            {
                               if(maps_set["defaultmap"])
                               {
                                  current_map = maps_set["defaultmap"];
                               }
                               else
                               {
                                  current_map = maps_set["list"][0];
                               }
                               ctrlr_map.load_map(current_map,"map",false);
                               set_mapselection(current_map, menu_data);
                            }
                            // $('html').css({ cursor: 'none' });
                         },
                         function(userdata) {
                         },
                         false);
         });
      });
   },
   function() { console.log("socketio error"); }
   );
});

</script>
<style>
.mp_overflow_hidden {
   overflow: hidden;
}
.mp_overflow_scroll {
   overflow: scroll;
}

.navpanel {
   width:600px;
   height:50px;
   line-height:50px;
   top: 50px;
   left: 800px;
   opacity: 0.7;
   background-color:#ccc; 
   border-radius: 5px;
   position: relative;
   z-index: 1000;
}
</style>
<body style="width:100%;height:100%;margin:0;padding:0;overflow:hidden">

   <div id="container_mp" class='mp_overflow_scroll' style="position:absolute;width:100%;height:100%;background:#EEEEEE">
      <div id="map_mp" style="position:relative;overflow:auto;background:#FFFFFF">
      </div>
   </div> 

   <div id="widgets_container_mp" style="display:none"></div>

   <div id="map_cm_mp" class="easyui-menu" style="width:180px;display:hidden;">
      <div onclick="javascript:ctrlr_map._context_menu('load')">load</div>
      <div class="menu-sep"></div>
      <div onclick="javascript:overflow('hidden')">overflow:hidden</div>
      <div onclick="javascript:overflow('scroll')">overflow:scroll</div>
      <div class="menu-sep"></div>
      <div onclick="javascript:toAdmin()">admin</div>
   </div>

</body>
</html>
