<?php
//
//  SOUS-PAGE (SUB-VIEW) de index.php : statut de fonctionnement de mea-edomus 
//
include_once('../lib/configs.php');
session_start() ?>

<style>
.console
{
 font-family:verdena,arial;
 font-size:12px;
 border:1px dotted black;
 overflow-y:scroll;
}
</style>

<div id="console" class="console" style="width:600px; min-height:200px; height:200px;">
</div>


<script type="text/javascript" src="lib/js/mea-auth-utils.js"></script>
<script type="text/javascript" src="lib/js/mea-gui-utils.js"></script>

<script type="text/javascript">

   var whoIsScrollingFlag=0; // 1 = le script, 0 = l'utilisateur
   var scrollOnOffFlag=1;    // 1 = activé par defaut
 
   function toLogConsole(line)
   {
      // analyse de la ligne
      var color="black"; // couleur par défaut
      if(line.indexOf("ERROR:")==0)
         color="red";
      else if(line.indexOf("WARNING:")==0)
         color="orange";
      else if(line.indexOf("INFO:")==0)
         color="green";
      else if(line.indexOf("DEBUG:")==0)
         color="blue";
     
      // ajout de la ligne
      $('#console').append("<div class='log' style='color:"+color+";'>"+line+"</div>");
     
      // on retire une ligne si la limite est atteinte
      var logLineDivHigh = 0;
      if($('#console > *').length >= 30)
      {
         var toRemove = $("#console div:first-child"); // ligne à retirer
         whoIsScrollingFlag=1;
         logLineDivHigh =toRemove.height(); // hauteur de la ligne à retirer
         toRemove.remove(); // on la retire
      }

      if(scrollOnOffFlag==1) { // le scroll est actif
         whoIsScrollingFlag=1; // on dis qu'on va générer un événement de scroll (il faudra ne pas en tenir compte)
         $('#console').scrollTop($('#console').prop("scrollHeight"));
      }
      else
      {
         // le scroll n'est pas actif
         if(logLineDivHigh) // si une ligne a été retirée
            $("#console").scrollTop($("#console").scrollTop()-logLineDivHigh); // je remonte le slider de la hauteur de la ligne retirée
      }
   }


   function socketio_available() { // socket io est chargé, on se connecte 
      var socketio_addr=window.location.protocol + '//' + window.location.hostname + ':8000';
      var socket = io.connect(socketio_addr);

      $("#console").scroll(function() {
         if(scrollOnOffFlag==0 && whoIsScrollingFlag==0) // si scroll inactif
         {
            // le slider a-t-il été poussé jusqu'en bas ?
            if($(this).scrollTop() + $(this).innerHeight() >= this.scrollHeight) {
               scrollOnOffFlag=1; // on réactive le scroll (live)
            }
         }
         else if(whoIsScrollingFlag==0) // le scroll est activé et c'est l'utilisateur qui à scroller
         {
            scrollOnOffFlag=0; // dans ce cas on désactive le scroll
         }
         else
            whoIsScrollingFlag=0; // remise à 0 du flag.
      });


      socket.on('log', function(message){
         toLogConsole(line)
      });
   }

   
   function socketio_unavailable(jqXHR, textStatus, errorThrown) {
      jqXHR.abort();
      $("#info").text("Pas d'iosocket => pas de console en live ...");
   }  

   jQuery(document).ready(function(){
      var socketiojs_url=window.location.protocol + '//' + window.location.hostname + ':8000/socket.io/socket.io.js';
      
      $('#titre').append(socketiojs_url);
      $.ajax({
         url: socketiojs_url,
         dataType: "script",
         timeout: 5000,
         success: socketio_available,
         error: socketio_unavailable
      });   
   });

</script> 