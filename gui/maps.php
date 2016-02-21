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


function padzero(s, size)
{
   var o = s;
   while (o.length < (size || 2)) {o = "0" + o;}

   return o;
}


jQuery(document).ready(function(){
   $.ajaxSetup({ cache: false });

<?php
   $IP=getenv('REMOTE_ADDR');
   echo "LANG='$LANG';";
   echo "var ip='$IP';";

   if(isset($_REQUEST['view'])) {
      echo "var destview=\""; echo $_REQUEST['view']; echo "\";\n";
   }
   else {
      echo "var destview=\"\";\n";
   }
   echo "var socketio_port="; echo $IOSOCKET_PORT; echo ";\n";
?>
   ip = ip.split(".").map(function(x){return parseInt(x)});
   var hexIP = padzero(ip[0].toString(16),2)
             + padzero(ip[1].toString(16),2)
             + padzero(ip[2].toString(16),2)
             + padzero(ip[3].toString(16),2);
   console.log (hexIP.toUpperCase());

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
      var list = [
         "../widgets/meawidget_value.js",
         "../widgets/meawidget_slider.js",
         "../widgets/meawidget_pastille.js",
         "../widgets/meawidget_button.js"
      ];

      ctrlr_map = new MapController(
         "container_mp",
         "map_mp",
         "map_cm_mp",
         "widgets_container_mp");
      ctrlr_map.linkToTranslationController(translationController);
      ctrlr_map.linkToCredentialController(credentialController);

      ctrlr_map.start();

      if(s!=null) {
         var aut_listener=ctrlr_map.__aut_listener.bind(ctrlr_map);
          s.on('aut', aut_listener);
      }
      else {
      }

      ctrlr_map.loadWidgets(list, function() {
         ctrlr_map.load_map("example1.1","map",false);
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
