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
 overflow-y:scroll;
}

.cadre
{
 width:550px;
}
 
.pastille
{
 height:40px;
 width:40px;
 margin-right:15px;
 margin-left:15px;
}
 
.process
{
 height:40px;
 line-height:40px;
}
 
.bouton
{
 height:40px;
 line-height:40px;
 text-align:center;
}

.titre
{
 height:30px;
 line-height:30px;
}

</style>

<div id="tabs1">
<ul>
<li><a href="#tabs1-1" id='synthese'>Synthèse</a></li>
<li><a href="#tabs1-3" id='interfaces'>Interfaces</a></li>
<li><a href="#tabs1-2" id='processes'>Processus</a></li>
<li><a href="#tabs1-4" id='log'>Log</a></li>
</ul>

<div id="tabs1-1">
</div>

<div id="tabs1-2">
   <p></p>
   <div style="margin:auto;" class="cadre ui-widget ui-widget-content ui-corner-all">
   <div class="ui-widget-header titre">
      <div style="margin-left:20px;">Gestion des processus de mea-edomus</div>
   </div>
      <p></p>
      <div>
         <table id="table_processes">
            <tbody>
            </tbody>
         </table>
      </div>
      <p></p>
   </div>
   <p></p>
</div>

<div id="tabs1-3">
   <p></p>
   <div style="margin:auto;" class="cadre ui-widget ui-widget-content ui-corner-all">
   <div class="ui-widget-header titre">
      <div style="margin-left:20px;">Rechargement des configurations des interfaces</div>
   </div>
      <p></p>
      <div>
         <table id="table_reload">
            <tbody>
            </tbody>
         </table>
      </div>
      <p></p>
   </div>
   <p></p>
   <div style="margin:auto;" class="cadre ui-widget ui-widget-content ui-corner-all">
   <div class="ui-widget-header titre">
      <div style="margin-left:20px;">Gestion des interfaces</div>
   </div>
      <p></p>
      <div id="montest">
         <table id="table_interfaces">
            <tbody>
            </tbody>
         </table>
      </div>
      <p></p>
   </div>
</div>

<div id="tabs1-4">
  <div class="ui-corner-all ui-widget-content" style="padding: 10px;">
     <div id="console" class="console" style="margin:auto; min-height:400px; height:400px;"></div>
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
   liveCom.connect(socketio_port);
   
   $( "#tabs1" ).tabs();
   start_index_controller(socketio_port);
});
</script>