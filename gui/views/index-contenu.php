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
 height:24px;
 width:24px;
 margin-right:10px;
 margin-left:10px;
 border-radius: 24px;
}
 
.process
{
// border: dotted 1px black;
 height:40px;
 line-height:40px;
}
 
.bouton
{
// border: dotted 1px red;
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

<style>
.wait_ball {
    background-color: rgba(0,0,0,0);
    border: 5px solid rgba(75, 165, 202,0.9);
    opacity: .9;
    border-top: 5px solid rgba(0,0,0,0);
    border-left: 5px solid rgba(0,0,0,0);
    border-radius: 32px;
    box-shadow: 0 0 3px #2187e7;
    width: 32px;
    height: 32px;
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
</style>

<div id="tabs1">
<ul>
<li><a href="#tabs1-1" id='synthese'>Synthèse</a></li>
<li><a href="#tabs1-3" id='interfaces'>Interfaces/processus</a></li>
<li><a href="#tabs1-4" id='log'>Journal</a></li>
</ul>

<div id="tabs1-1">
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
      <div id="montest">
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