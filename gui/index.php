<?php
//
//  PAGE PRINCIPALE (VIEW) : home page
//
include_once('lib/configs.php');
include_once('lib/php/translation.php');
include_once('lib/php/$LANG/translation.php');
mea_loadTranslationData($LANG,'');
session_start();
?>

<!DOCTYPE html>

<?php
// contrôle et redirections
if(!isset($_SESSION['logged_in']))
{
    $dest=$_SERVER['PHP_SELF'];
    echo "<script>window.location = \"login.php?dest=$dest\";</script>";
    exit();
}
?>

<html>
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
   <link rel='stylesheet' href='lib/bgrins-spectrum/spectrum.css' />
</head>
   
<script src="lib/ace/src-noconflict/ace.js" type="text/javascript"></script>

<script type="text/javascript" src="lib/jquery-easyui-1.4.4/jquery.min.js"></script>

<script type="text/javascript" src="lib/jquery-easyui-1.4.4/jquery.easyui.min.js"></script>
<script type="text/javascript" src="lib/jquery-easyui-datagridview/datagrid-groupview.js"></script>

<script type="text/javascript" src="lib/noty-2.2.10/js/noty/packaged/jquery.noty.packaged.min.js"></script>

<script src="lib/bgrins-spectrum/spectrum.js" type="text/javascript"></script>

<script type="text/javascript" src="lib/highstock-4.2.3/js/highstock.js"></script>
<script type="text/javascript" src="lib/highcharts-4.2.3/js/highcharts-more.js"></script>
<script type="text/javascript" src="lib/highcharts-4.2.3/js/modules/solid-gauge.js"></script>
<script type="text/javascript" src="lib/highcharts-4.2.3/js/modules/exporting.js"></script>
<script type="text/javascript" src="lib/highcharts-4.2.3/js/modules/offline-exporting.js"></script>
<script type="text/javascript" src="lib/highcharts-4.2.3/js/modules/no-data-to-display.js"></script>
<script type="text/javascript" src="lib/highcharts-4.2.3/js/themes/grid-light.js"></script>

<!-- Chargement des modules et objets communs -->
<script type="text/javascript" src="models/common/models-utils.js"></script>

<script type="text/javascript" src="controllers/common/meaobject.js"></script>
<script type="text/javascript" src="controllers/common/commoncontroller.js"></script>
<script type="text/javascript" src="controllers/common/gridcontroller.js"></script>
<script type="text/javascript" src="controllers/common/translationcontroller.js"></script>
<script type="text/javascript" src="controllers/common/credentialcontroller.js"></script>
<script type="text/javascript" src="controllers/common/livecomcontroller.js"></script>
<script type="text/javascript" src="controllers/common/livelogcontroller.js"></script>
<script type="text/javascript" src="controllers/common/viewscontroller.js"></script>
<script type="text/javascript" src="controllers/common/tabspagecontroller.js"></script>
<script type="text/javascript" src="controllers/common/filechoosercontroller.js"></script>
<script type="text/javascript" src="controllers/common/filechooseruploadercontroller.js"></script>
<script type="text/javascript" src="controllers/common/permmemcontroller.js"></script>
<script type="text/javascript" src="controllers/common/orderedFileslistselector.js"></script>

<script type="text/javascript" src="widgets/meawidget.js"></script>

<!-- surcharge des méthodes du controleur de traduction spécifiques à une langue donnée -->
<?php
   echo "<script type='text/javascript' src='lib/js/".$LANG."/mea-translation.js'></script>"; // extension
?>

<script>
// fonctions "hors" controleur
function logout()
{
   $.get('models/destroy_php_session.php',
      {},
      function(data){
      // controler le retour du serveur ...
      },
      "json"
   )
   .always(function(){ window.location = "login.php"; });
}


// lancement du controleur sans les communications "live" (réduction des fonctions disponibles)
function liveComUnavailable(destview)
{
   $('#aa').accordion('remove',translationController.toLocalC('supervision')); // pas menu nécessitant des communications lives
   $('#mea-layout').layout('collapse','south'); // console masquée
   $('#console').text(translationController.toLocalC('No live information available !')); // un peu d'info quand même
   
   viewsController.addView(translationController.toLocalC('sensors/actuators'),'page2.php','page2_tab');
   viewsController.addView(translationController.toLocalC('interfaces'),'page2.php','page2_tab');
   viewsController.addView(translationController.toLocalC('types'),'page2.php','page2_tab');
   viewsController.addView(translationController.toLocalC('locations'),'page2.php','page2_tab');

   viewsController.addView(translationController.toLocalC('application'),'page3.php','page3_tab');
   viewsController.addView(translationController.toLocalC('users'),'page3.php','page3_tab');

   viewsController.addView(translationController.toLocalC('rules editor'),'page4.php','page4_tab');
   viewsController.addView(translationController.toLocalC('rules manager'),'page4.php','page4_tab');

   viewsController.addView(translationController.toLocalC('map editor'),'page5.php','page5_tab');
   viewsController.addView(translationController.toLocalC('maps set editor'),'page5.php','page5_tab');

   if(destview=="" || typeof(viewController.views[destview])=="undefined")
      destview=translationController.toLocalC('sensors/actuators');
   
   viewsController.displayView(destview);
}


// lancement du contrôleur avec livecom.
function liveComAvailable(s,destview)
{
   liveLogController=new LiveLogController("console",1000);
  
   viewsController.addView(translationController.toLocalC('indicators'),'page1.php','page1_tab');
   viewsController.addView(translationController.toLocalC('services'),'page1.php','page1_tab');
   viewsController.addView(translationController.toLocalC('sensors/actuators'),'page2.php','page2_tab');
   viewsController.addView(translationController.toLocalC('interfaces'),'page2.php','page2_tab');
   viewsController.addView(translationController.toLocalC('types'),'page2.php','page2_tab');
   viewsController.addView(translationController.toLocalC('locations'),'page2.php','page2_tab');

   viewsController.addView(translationController.toLocalC('application'),'page3.php','page3_tab');
   viewsController.addView(translationController.toLocalC('users'),'page3.php','page3_tab');

   viewsController.addView(translationController.toLocalC('rules editor'),'page4.php','page4_tab');
   viewsController.addView(translationController.toLocalC('rules manager'),'page4.php','page4_tab');

   viewsController.addView(translationController.toLocalC('map editor'),'page5.php','page5_tab');
   viewsController.addView(translationController.toLocalC('maps set editor'),'page5.php','page5_tab');

   if(destview=="" || typeof(viewsController.views[destview])=="undefined")
      destview=translationController.toLocalC('indicators');

   viewsController.displayView(destview);
   liveLogController.start(s);
}


function resizeDiv()
{
   vpw = $(window).width();
   vph = $(window).height();
   vph = vph - 130;
   if(vph < 670)
      vph=670;
   $('#mea-layout').layout('resize', { width:'100%', height:vph });
}


jQuery(document).ready(function() {
   $.ajaxSetup({ cache: false });

   Highcharts.setOptions({
      global: {
         useUTC: false
      }
   });

   resizeDiv();
   $(window).resize(function() {
      resizeDiv();
   });

   //
   // récupération des variables depuis PHP
   //
<?php
   echo "LANG='$LANG';";

   if(isset($_REQUEST['view'])) {
      echo "var destview=\""; echo $_REQUEST['view']; echo "\";\n";
   }
   else {
      echo "var destview=\"\";\n";
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

   // controleur memoire
   permMemController = new PermMemController();

   // initialisation du contrôle de vues
   viewsController = new ViewsController();   

   // initialisation et lancement des communications "live"
   liveComController = new LiveComController(socketio_port);
   
   // lancement du controleur de communication live (si possible)
   liveComController.start(
         function(s){liveComAvailable(s,destview);},
         function() {liveComUnavailable(destview);}
   );

   // propagation d'évenement sur le paneau central
   $('#mea-layout').layout('panel','center').panel({
      onResize: function() {
         $(document).trigger( "MeaCenterResize", [ "", "" ] );
      },
      onBeforeLoad: function() {
         $(document).trigger( "MeaCleanView", [ "", "" ] );
      }
   });
});
</script>

<style>
a.meamenu {
   color:black;
   text-decoration:none;
}

a.meamenu:visited {
}

a.meamenu:hover {
   font-weight:bold;
}

.console {
   border: dotted 1px gray;
   font-family:"Lucida Console", Monaco, monospace;
   font-size:12px;
   overflow-y:scroll;
}
</style>

<body style="margin:0;padding:0;height:100px;width:100%;overflow:auto">
   <div style="min-width:950px;">
      <div id='logo'  style="float:left; width:250px; height:50px; text-align:left;">LOGO</div>
      <div id='pub' style="width:250px; float:right; height:50px; text-align:right;">INFORMATION</div>
      <div id='titre-page' style="height:50px; text-align:center;">CENTER</div>
   </div>
   <div id='meamain' style="min-width:950px;min-height:650px;margin:10px;">
<!--
      <div id='mea-layout' class="easyui-layout" fit=true>
-->
      <div id='mea-layout' class="easyui-layout">

         <div region="west" split="true" collapsible="false" title="" style="width:200px;">
            <div id="aa" class="easyui-accordion" data-options="animate:false,border:false" style="width:100%;">
            
               <div title="<?php mea_toLocalC('maps'); ?>" style="overflow:auto;padding:10px;">
                  <div><a href="#" class="meamenu" onclick="javascript: window.location='maps.php'"><?php mea_toLocalC('full screen maps'); ?></a></div>
                  <div><a href="#" class="meamenu" onclick="javascript:viewsController.displayView(translationController.toLocalC('map editor'),'page5.php','page5_tab')"><?php mea_toLocalC('map editor'); ?></a></div>
                  <div><a href="#" class="meamenu" onclick="javascript:viewsController.displayView(translationController.toLocalC('maps set editor'),'page5.php','page5_tab')"><?php mea_toLocalC('maps set editor'); ?></a></div>
               </div>

               <div title="<?php mea_toLocalC('automator'); ?>" style="overflow:auto;padding:10px;">
                  <div><a href="#" class="meamenu" onclick="javascript:viewsController.displayView(translationController.toLocalC('rules manager'),'page4.php','page4_tab')"><?php mea_toLocalC('rules manager'); ?></a></div>
                  <div><a href="#" class="meamenu" onclick="javascript:viewsController.displayView(translationController.toLocalC('rules editor'),'page4.php','page4_tab')"><?php mea_toLocalC('rules editor'); ?></a></div>
               </div>

               <div title="<?php mea_toLocalC('inputs/outputs'); ?>" style="overflow:auto;padding:10px;">
                  <div><a href="#" class="meamenu" onclick="javascript:viewsController.displayView(translationController.toLocalC('sensors/actuators'),'page2.php','page2_tab')"><?php mea_toLocalC('sensors/actuators'); ?></a></div>
                  <div><a href="#" class="meamenu" onclick="javascript:viewsController.displayView(translationController.toLocalC('interfaces'),'page2.php','page2_tab')"><?php mea_toLocalC('interfaces'); ?></a></div>
                  <div><a href="#" class="meamenu" onclick="javascript:viewsController.displayView(translationController.toLocalC('types'),'page2.php','page2_tab')"><?php mea_toLocalC('types'); ?></a></div>
                  <div><a href="#" class="meamenu" onclick="javascript:viewsController.displayView(translationController.toLocalC('locations'),'page2.php','page2_tab')"><?php mea_toLocalC('locations'); ?></a></div>
               </div>

               <div id='forlivecom' title="<?php mea_toLocalC('supervision'); ?>" data-options="selected:true" style="overflow:auto;padding:10px;">
                  <div><a href="#" class="meamenu" onclick="javascript:viewsController.displayView(translationController.toLocalC('indicators'),'page1.php','page1_tab')"><?php mea_toLocalC('indicators'); ?></a></div>
                  <div><a href="#" class="meamenu" onclick="javascript:viewsController.displayView(translationController.toLocalC('services'),'page1.php','page1_tab')"><?php mea_toLocalC('services'); ?></a></div>
               </div>
               
               <div title="<?php mea_toLocalC('preferences'); ?>" style="overflow:auto;padding:10px;">
                  <div><a href="#" class="meamenu" onclick="javascript:viewsController.displayView(translationController.toLocalC('application'),'page3.php','page3_tab')"><?php mea_toLocalC('application'); ?></a></div>
                  <div><a href="#" class="meamenu" onclick="javascript:viewsController.displayView(translationController.toLocalC('users'),'page3.php','page3_tab')"><?php mea_toLocalC('users'); ?></a></div>

               </div>
               <div title="<?php mea_toLocalC('session'); ?>" style="overflow:auto;padding:10px;">
                  <div><a href="#" class="meamenu" onclick="javascript:logout()"><?php mea_toLocalC('logout'); ?></a></div>
               </div>
            </div>
         </div>

         <div id="content" region="center" title=""></div>
         <div id="livelog" region="south" split="true" collapsible="true" title="<?php mea_toLocalC('live log'); ?>" style="height:150px;position:relative;overflow:hidden">
            <div id="console" class="console" style="background:lightgray;width:auto;height:100%"></div>
         </div>
      </div>
   </div>
   <div style="font-size: 10px; text-align:center;">
   (c) 2014 Mea soft
   </div>
</body>
</html>
