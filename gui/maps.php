<?php
include_once('lib/configs.php');
$DEBUG_ON=1;
if(isset($DEBUG_ON) && ($DEBUG_ON == 1))
{
   ob_start();
   print_r($_REQUEST);
   $debug_msg = ob_get_contents();
   ob_end_clean();
   error_log($debug_msg);
}

$autologin=False;
if(isset($_REQUEST['autologin']))
{
   $autologin=$_REQUEST['autologin'];
}

session_start();

if(isset($_SESSION['language']))
{
   $LANG=$_SESSION['language'];
}
include_once('lib/php/translation.php');
include_once('lib/php/$LANG/translation.php');
mea_loadTranslationData($LANG,'');
?>

<!DOCTYPE html>

<?php
// contrôle et redirections
if(!isset($_SESSION['logged_in']))
{
    $dest=$_SERVER['PHP_SELF'];
    if($autologin<>False)
       echo "<script>window.location = \"login.php?dest=$dest&autologin=$autologin\";</script>";
    else
       echo "<script>window.location = \"login.php?dest=$dest\";</script>";
    exit();
}
?>

<html style="overflow: hidden">
<head>
   <title>
   <?php echo $TITRE_APPLICATION;?>
   </title>
   <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
   <meta name="viewport" content="width=device-width, initial-scale=1, minimun-scale=1.0, maximun-scale=1.0, user-scalable=no">
   <meta name="apple-mobile-web-app-capable" content="yes" />
   <meta name="description" content="domotique DIY !">

   <link rel="stylesheet" type="text/css" href="lib/jquery-easyui-1.4.4/themes/default/easyui.css">
   <link rel="stylesheet" type="text/css" href="lib/jquery-easyui-1.4.4/themes/icon.css">
   <link rel="stylesheet" type="text/css" href="lib/jquery-easyui-1.4.4/themes/color.css">
   <link rel="stylesheet" type="text/css" href="lib/mea-edomus.css">
   <style>
      html,body{ margin:0;padding:0;height:100%;width:100%; }

      div::-webkit-scrollbar {
          -webkit-appearance: none;
          width: 7px;
      }
      div::-webkit-scrollbar-thumb {
          border-radius: 4px;
          background-color: rgba(0,0,0,.5);
          -webkit-box-shadow: 0 0 1px rgba(255,255,255,.5);
      }
      .mp_overflow_hidden {
         overflow-x: hidden;
      }
      .mp_overflow_scroll {
         overflow-x: scroll;
      }
      .mp_navpanel {
         width:600px;
         height:50px;
         line-height:50px;
         opacity: 0.7;
         background-color:#ccc;
         border-radius: 5px;
         position: relative;
         z-index: 1000;
      }
      .spinner {
          background-position: center center;
          background-repeat: no-repeat;
          border-color:transparent;
          border-radius: 50%;
          opacity: .7;
      }
      .spinner.large {
          height: 30px; width: 30px;
          background: url("lib/wait.gif");
      }
   </style>
</head>
<body style="overflow:hidden;position:fixed">
   <div id="container_mp" class='mp_overflow_hidden' style="position:absolute;width:100%;height:100%;background:#EEEEEE">
      <div id="map_wait_mp">
         <div style="position:absolute;top:40%;left:40%;height:20%;width:20%;display:table">
            <div style="display:table-cell;text-align:center;vertical-align: middle;">
               <div class="spinner large">
               </div>
            </div>
         </div>
      </div>
      <div id="map_mp" style="position:relative;overflow:auto;background:#FFFFFF">
      </div>
   </div>

   <div id="widgets_container_mp" style="display:none">
   </div>

   <div id="map_cm_mp" class="easyui-menu" style="width:180px;display:hidden;">
<!--
      <div onclick="javascript:ctrlr_map._context_menu('load')">load</div>
-->
      <div onclick="javascript:window.location='maps.php'">load</div>
      <div class="menu-sep"></div>
      <div onclick="javascript:overflow('hidden')">overflow:hidden</div>
      <div onclick="javascript:overflow('scroll')">overflow:scroll</div>
      <div class="menu-sep"></div>
      <div onclick="javascript:toAdmin()">admin</div>
      <div class="menu-sep"></div>
      <div onclick="javascript:logout()">logout</div>
   </div>
</body>

<script type="text/javascript" src="lib/jquery-easyui-1.4.4/jquery.min.js"></script>
<script type="text/javascript" src="lib/jquery-easyui-1.4.4/jquery.easyui.min.js"></script>
<script type="text/javascript" src="lib/noty-2.2.10/js/noty/packaged/jquery.noty.packaged.min.js"></script>

<script type="text/javascript" src="lib/highstock-4.2.3/js/highstock.js"></script>
<script type="text/javascript" src="lib/highstock-4.2.3/js/highcharts-more.js"></script>
<script type="text/javascript" src="lib/highstock-4.2.3/js/modules/solid-gauge.js"></script>
<script type="text/javascript" src="lib/highstock-4.2.3/js/modules/exporting.js"></script>
<script type="text/javascript" src="lib/highstock-4.2.3/js/modules/offline-exporting.js"></script>
<script type="text/javascript" src="lib/highstock-4.2.3/js/modules/no-data-to-display.js"></script>
<script type="text/javascript" src="lib/highstock-4.2.3/js/themes/grid-light2.js"></script>


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
   window.location = "index.php";
}


function logout()
{
   $.get('models/destroy_php_session.php',
      {},
      function(data){
      },
      "json"
   )
   .always(function(){ window.location = "login.php"; });
}


function overflow(s)
{
   console.log($('body').outerWidth()+" "+$('body').outerHeight());
   console.log($('body').innerWidth()+" "+$('body').innerHeight());
   console.log($("#container_mp").outerWidth()+" "+$("#container_mp").outerHeight());
   console.log($("#container_mp").offset().top+" "+$("#container_mp").offset().left);
   
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

Highcharts.setOptions({
    chart: {
        style: {
            fontFamily: 'sans-serif'
        }
    }
});

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
         setInterval(function(){$.post('models/refresh_php_session.php');},600000);

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

      $(window).on('orientationchange', function(e) {
         navigationPanel.redraw();
         $.mobile.changePage(window.location.href, {
            allowSamePageTransition: true,
            transition: 'none',
            reloadPage: true
         });
      });
   },
   function() { console.log("socketio error"); }
   );
});

</script>
</html>
