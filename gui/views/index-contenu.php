<?php
//
//  SOUS-PAGE (SUB-VIEW) de index.php : statut de fonctionnement de mea-edomus 
//
include_once('../lib/configs.php');
session_start() ?>

<style>
.console
{
 font-family:"Lucida Console", Monaco, monospace;
 font-size:12px;
 border:1px dotted black;
 overflow-y:scroll;
}
</style>

<div id="tabs1">
<ul>
<li><a href="#tabs1-1" id='synthese'>Synthèse</a></li>
<li><a href="#tabs1-2" id='indicateurs'>Indicateurs</a></li>
<li><a href="#tabs1-3" id='log'>Log</a></li>
</ul>

<div id="tabs1-1">
</div>

<div id="tabs1-2">
   <div id="info">UNE INFO</div>
</div>

<div id="tabs1-3">
  <div class="ui-corner-all ui-widget-content" style="padding-top: 40px; padding-bottom: 40px;">
     <div id="console" class="console" style="margin:auto; min-width:800px; min-height:400px; height:400px;"></div>
  </div>
</div>


<script type="text/javascript" src="lib/js/mea-auth-utils.js"></script>
<script type="text/javascript" src="lib/js/mea-gui-utils.js"></script>
<script type="text/javascript" src="controllers/index.js"></script>

<script>
jQuery(document).ready(function(){
<?php
   echo "var socketio_port=";
   echo $IOSOCKET_PORT;
   echo ";\n";
   ?>

   $( "#tabs1" ).tabs();
   start_index_controller(socketio_port);
});
</script>