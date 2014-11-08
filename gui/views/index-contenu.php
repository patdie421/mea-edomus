<?php
//
//  SOUS-PAGE (SUB-VIEW) de index.php : statut de fonctionnement de mea-edomus 
//
include_once('../lib/configs.php');
session_start() ?>

<style>
 
.console {
 font-family:"Lucida Console", Monaco, monospace;
 font-size:12px;
 overflow-y:scroll;
}
 
.cadre {
 width:700px;
}

.pastille {
 height:24px;
 width:24px;
 margin-right:10px;
 margin-left:10px;
 border-radius: 24px;
}

.process {
 max-height: 40px;
 margin: auto;
}
 
.name {
 font-size:14px;
 max-height:20px;
 line-height:100%;
 overflow: hidden;
}
 
.description {
 line-height:100%;
 max-height:17px;
 font-size:10px;
 overflow: hidden;
}
 
.bouton {
// border: dotted 1px red;
 height:40px;
 line-height:40px;
 text-align:center;
}
 
.titre {
 height:30px;
 line-height:30px;
}
 
.display {
 display:none;
}
 
.conteneur {
 min-height:500px;
 height:500px;
 line-height:500px;
 text-align:center; /* centrage horizontal */
}
 
.contenu {
 vertical-align:middle;
 display:inline-block;
 line-height:auto;
}
 
.table_processes {
 border-collapse:collapse;
}

.table_indicateurs {
 border-collapse:collapse;
// border-width:10px;
// border-style:solid;
// border-color:#CDCDCD;
 }

.cell_table_indicateurs {
 border-width:1px;
 padding:0px;
 border-style:solid;
 border-bottom:none;
 border-color:#CDCDCD;
}

.cell_table_indicateurs:first-child {
 border-left:none;
}

.cell_table_indicateurs:last-child {
 border-right:none;
}

.indicators_list
 {
 margin-bottom: -1px;
 border-collapse:collapse;
 }

.processes_column {
 min-width:250px;
 }
 
.status_column {
 min-width:50px;
}

.columns_indicators {
}

.process_name {
 margin-left:10px;
}
 
.process_description {
 font-size:10px;
 margin-left:10px;
}
 
.indicator_name {
 margin: 0px;
 padding: 7px;
 min-width: 200px;
 width: 200px;
}
 
.indicator_value {
 margin: 0px;
 padding: 7px;
 text-align: right;
 min-width: 100px;
 width: 100%;
}
 
.couleur_1 {
 background:#d6f9b3;
}
 
.couleur_2 {
 background: #cbefa8;
}
 
.wait_ball {
 background-color: rgba(0,0,0,0);
 border: 5px solid rgba(75, 165, 202,0.9);
 opacity: .9;
 border-top: 5px solid rgba(0,0,0,0);
 border-left: 5px solid rgba(0,0,0,0);
 border-radius: 30px;
 box-shadow: 0 0 3px #2187e7;
 width: 30px;
 height: 30px;
 margin: 0 auto;
 -moz-animation: spin .75s infinite linear;
 -webkit-animation: spin .75s infinite linear;
}
 
@-moz-keyframes spin {
 0% {
  -moz-transform: rotate(0deg);
 }
 100% {
  -moz-transform: rotate(360deg);
 };
}
 
@-moz-keyframes spinoff {
 0% {
  -moz-transform: rotate(0deg);
 }
 100% {
  -moz-transform: rotate(-360deg);
 };
}
 
@-webkit-keyframes spin {
 0% {
  -webkit-transform: rotate(0deg);
 }
 100% {
  -webkit-transform: rotate(360deg);
 };
}
 
@-webkit-keyframes spinoff {
 0% {
  -webkit-transform: rotate(0deg);
 }
 100% {
  -webkit-transform: rotate(-360deg);
 };
}

@-moz-keyframes spin {
 0% {
  -moz-transform: rotate(0deg);
 }
 100% {
  -moz-transform: rotate(360deg);
 };
}

@-moz-keyframes spinoff {
 0% {
  -moz-transform: rotate(0deg);
 }
 100% {
  -moz-transform: rotate(-360deg);
 };
}

@-webkit-keyframes spin {
 0% {
  -webkit-transform: rotate(0deg);
 }
 100% {
  -webkit-transform: rotate(360deg);
 };
}

@-webkit-keyframes spinoff {
 0% {
  -webkit-transform: rotate(0deg);
 }
 100% {
  -webkit-transform: rotate(-360deg);
 };
}
</style>


<div id="wait" class="display ui-corner-all ui-widget-content">
   <div class="conteneur">
      <div class="contenu">
         <div class="wait_ball" style="margin:auto;"></div>
      </div>
   </div>
</div>

<div id="unavailable" class="display ui-corner-all ui-widget-content">
   <div class="conteneur">
      <div class="contenu">
      live communication unavailable
      </div>
   </div>
</div>

<div id="available" class="display">
   <div id="tabs1">
   <ul>
   <li><a href="#tabs1-1" id='synthese'>Fonctionnement</a></li>
   <li><a href="#tabs1-3" id='interfaces'>Contrôle</a></li>
   <li><a href="#tabs1-4" id='log'>Journal</a></li>
   </ul>

   <div id="tabs1-1">
      <p></p>
      <div id="indicators" style="margin:auto;" class="cadre ui-widget ui-widget-content ui-corner-all">
         <div class="ui-widget-header ui-corner-top titre">
            <div style="margin-left:20px;">Indicateurs de fonctionnement</div>
         </div>
         <table id="table_indicateurs" class="table_indicateurs" style="width:100%;">
         </table>
         <div id="noindicators" class="display">
            No indicators provided
         </div>
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
            <table id="table_reload" style="width:100%;">
               <tbody>
               </tbody>
            </table>
         </div>
         <p></p>
      </div>
      <p></p>
      <div style="margin:auto;" class="cadre ui-widget ui-widget-content ui-corner-all">
         <div class="ui-widget-header titre">
            <div style="margin-left:20px;">Etat des interfaces</div>
         </div>
         <p></p>
         <div>
            <table id="table_interfaces" style="width:100%;">
               <tbody>
               </tbody>
            </table>
         </div>
         <p></p>
      </div>
      <p></p>
      <div style="margin:auto;" class="cadre ui-widget ui-widget-content ui-corner-all">
         <div class="ui-widget-header titre">
            <div style="margin-left:20px;">Etat des processus de mea-edomus</div>
         </div>
         <p></p>
         <div>
            <table id="table_processes" style="width:100%;">
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
   $("#wait").show();
   liveCom.connect(socketio_port);
   start_index_controller(socketio_port);
});
</script>