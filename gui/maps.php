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
<script type="text/javascript" src="controllers/navigationpanelcontroller.js"></script>

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


jQuery(document).ready(function(){
   $.ajaxSetup({ cache: false });

var map = false;
var mapsset = false;

<?php
//   $IP=getenv('REMOTE_ADDR');
   echo "LANG='$LANG';";

   if(isset($_REQUEST['map'])) {
      echo "map=\""; echo $_REQUEST['map']; echo "\";\n";
   }

   if(isset($_REQUEST['mapsset'])) {
      echo "mapsset=\""; echo $_REQUEST['mapsset']; echo "\";\n";
   }

   echo "var socketio_port="; echo $IOSOCKET_PORT; echo ";\n";
?>

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
   function(s) {
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

      var navigationPanel = new NavigationPanelController(ctrlr_map);
      navigationPanel.linkToTranslationController(translationController);
      var navigationPanelInit = navigationPanel.init.bind(navigationPanel);
 
      ctrlr_map.loadWidgets(function() {
         if(mapsset === false)
         {
            filechooser.open(ctrlr_map._toLocalC("choose maps set ..."),
               ctrlr_map._toLocalC("open"),
               ctrlr_map._toLocalC("open"),
               ctrlr_map._toLocalC("cancel"),
               "menu",
               false,
               false,
               "",
               navigationPanelInit,
               false);
         }
         else
         {
            navigationPanel.init(mapsset, "menu", false, map);  
         }
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
