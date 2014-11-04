var whoIsScrollingFlag=0; // 1 = le script, 0 = l'utilisateur
var scrollOnOffFlag=1;    // 1 = activé par defaut
var isadmin=1;

function ajax_error(xhr, ajaxOptions, thrownError){
    alert("index.js : responseText="+xhr.responseText+" status="+xhr.status+" thrownError="+thrownError);
}

/* ligne à ajouter si pas d'interface
var newRow = "<tr>" +
                "<td colspan=\"3\" style=\"width:100%; height:40px; text-align:center; border: dotted 1px green;\"><div>Aucne interface disponible</div></td>" +
             "</tr>";
$("#table_reload > tbody").before(newRow);
*/

function toLogConsole(line)
{
   // analyse de la ligne
   var color="black"; // couleur par défaut
   if(line.indexOf("ERROR")==0)
      color="red";
   else if(line.indexOf("WARNING")==0)
      color="orange";
   else if(line.indexOf("INFO")==0)
      color="green";
   else if(line.indexOf("DEBUG")==0)
      color="blue";
   if(line.lenght==0)
      return;
      
   // ajout de la ligne
   $('#console').append("<div class='log' style='white-space:pre-wrap; color:"+color+";'>"+line+"</div>");
     
   // on retire une ligne si la limite est atteinte
   var logLineDivHigh = 0;
   if($('#console > *').length >= 1000)
   {
      var toRemove = $("#console div:first-child"); // ligne à retirer
      whoIsScrollingFlag=1;
      logLineDivHigh = toRemove.height(); // hauteur de la ligne à retirer
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


function tabsActive(event,ui) {
   active = $("#tabs1").tabs("option", "active");
   if(active == 2) // activation de la console
   {
      // et on active le scroll auto
      scrollOnOffFlag=1;
      whoIsScrollingFlag=1;

      // on se met en bas de la log
      $("#console").scrollTop($("#console").height()); // on scroll à la fin de la div

      // ou :
      // $("#console").scrollIntoView(false);
   }
}


function anim_status(data)
{
   for(var key in data)
   {
      if(data[key]['status']==1) // process démarré
      {
         if(data[key]['heartbeat']=='KO' && data[key]['type']!=2) // type = 2 => tache donc pas de "black" possible
         {
            $("#process_"+data[key]['pid']).css("background","black");
         }
         else
         {
            $("#process_"+data[key]['pid']).css("background","green");
         }
         if(data[key]['type']!=2)
         {
            $("#bstart"+data[key]['pid']).button("option", "disabled", true );
            $("#bstop"+data[key]['pid']).button( "option", "disabled", false );
         }
      }
      else if(data[key]['status']==0 && data[key]['type']!=2) // non demarré
      {
         $("#process_"+data[key]['pid']).css("background","red");
         $("#bstart"+data[key]['pid']).button("option", "disabled", false );
         $("#bstop"+data[key]['pid']).button( "option", "disabled", true );
      }
      else
      {
         $("#process_"+data[key]['pid']).css("background","gray");
      }
   }
}


function add_row(table, name, desc, id, start_str, start, stop_str, stop, isadmin)
{
   newRow =  "<tr>" +
                 "<td style=\"width:10%;\"><div id=\"process_"+id+"\" class=\"pastille ui-widget ui-widget-content ui-corner-all\" style=\"background:gray;\"></div></td>" +
                 "<td style=\"width:60%;\">" +
                     "<div class=\"process\">" +
                        "<div class=\"name\">"+name+"</div>";
   if(desc!="" && desc!==undefined)
   {
                newRow+="<div class=\"description\">"+desc+"</div>";
   }
                newRow+="</div></td>" +
                 "<td style=\"width:30%;\">";
   if(isadmin==1)
   {
                newRow+="<div class=\"bouton\">";
      if(start != null)
      {
                   newRow+="<button id=\"bstart"+id+"\">"+start_str+"</button>";
      }
      if(stop != null)
      {
                   newRow+="<button id=\"bstop"+id+"\">"+stop_str+"</button>";
      }
                newRow+="</div>";
   }
         newRow+="</td>" +
             "</tr>";

   $("#"+table+" > tbody").before(newRow);
   $("#bstop"+id).button().click(function(event){stop(id);});
   $("#bstart"+id).button().click(function(event){start(id);});
};


function load_all_processes_lists(isadmin) {
   $.ajax({
      url: 'CMD/ps.php',
      async: true,
      type: 'GET',
      dataType: 'json',
      success: function(data){
         var nb=0;
         for(var key in data)
         {
            if(data[key]['type']==1)
               continue;
            if(data[key]['group']==0)
            {
              add_row("table_processes", key, data[key]['desc'], data[key]['pid'], "start", start, "stop", stop, isadmin);
            }
            else if(data[key]['group']==2)
            {
               add_row("table_reload", key, data[key]['desc'], data[key]['pid'], "reload", reload, null, null, isadmin);
            }
            else if(data[key]['group']==1)
            {
               nb++;
               add_row("table_interfaces", key, data[key]['desc'], data[key]['pid'], "start", start, "stop", stop, isadmin);
            }
         }
         if(nb==0)
            messageListeInterfaceVide();
         anim_status(data);
      },
      error: function(jqXHR, textStatus, errorThrown ){
         ajax_error( jqXHR, textStatus, errorThrown );
      }
   });
}


function messageListeInterfaceVide()
{
   var newRow="";
   newRow = "<tr>" +
               "<td style=\"width:100%; height:40px; text-align:center;\"><div>Aucune interface disponible</div></td>" +
            "</tr>";
   $("#table_interfaces > tbody").before(newRow);
}


function load_interfaces_list_only(isadmin) {
   $.ajax({
      url: 'CMD/ps.php',
      async: true,
      type: 'GET',
      dataType: 'json',
      success: function(data){
         var nb=0;
         for(var key in data)
         {
            if(data[key]['group']==1)
            {
               add_row("table_interfaces", key, data[key]['desc'], data[key]['pid'], "start", start, "stop", stop, isadmin);
               nb++;
            }
         }
         if(nb>0)
            anim_status(data);
         else
            messageListeInterfaceVide();
//         $("#bstart"+id).button("option", "disabled", false);
      },
      error: function(jqXHR, textStatus, errorThrown ){
         ajax_error( jqXHR, textStatus, errorThrown );
      }
   });
}


function checkError(data)
{
  errno=data["errno"];
  
  switch(errno)
  {
     case 0:
        return true;
        break;
     case 2: // pas habilité
        window.location = "index.php"; // rechargement de la page index qui décidera ce qu'il faut faire (cf. start_index_controler).
        return false;
        break;
     case 4: // param process non trouvé
        return false;
        break;
     case 5: // pas de parametre
        return false;
        break;
     case 6: // commande inconnue (doit être start/stop/reload)
        return false;
        break;
     case 7: // erreur de l'opération
        return false;
        break;
     case -1: // opération incohérente (déjà lancé ou déjà arrêté).
        return false;
        break;
     default:
      return false;
      break;
  }
}


function reload(id)
{
   $("#table_interfaces").empty();
   $("#table_interfaces").append("<tbody></tbody>");
   $("#table_interfaces > tbody").before("<tr><td style=\"width:100%; margin:auto;\" align=\"center\" valign=\"top\"><div class=\"wait_ball\"></div></td></tr>");
   // $("#bstart"+id).button("option", "disabled", true );
   $.ajax({
      url: 'CMD/startstop.php?process='+id+'&cmnd=task',
      async: true,
      type: 'GET',
      dataType: 'json',
      success: function(data){
         // $("#bstart"+id).button("option", "disabled", false );
         if(checkError(data)==false)
         {
            // on enleve juste la roue qui tourne ...
            $("#table_interfaces").empty();
            $("#table_interfaces").append("<tbody></tbody>");
         }

      },
      error: function(jqXHR, textStatus, errorThrown ){
         // $("#bstart"+id).button("option", "disabled", false );
         $("#table_interfaces").empty();
         $("#table_interfaces").append("<tbody></tbody>");
         ajax_error( jqXHR, textStatus, errorThrown );
      }
   });
}


function start(id)
{
   $.ajax({
      url: 'CMD/startstop.php?process='+id+'&cmnd=start',
      async: true,
      type: 'GET',
      dataType: 'json',
      success: function(data){
         checkError(data);
      },
      error: function(jqXHR, textStatus, errorThrown ){
         ajax_error( jqXHR, textStatus, errorThrown );
      }
   });
}


function stop(id)
{
   $.ajax({
      url: 'CMD/startstop.php?process='+id+'&cmnd=stop',
      async: true,
      type: 'GET',
      dataType: 'json',
      success: function(data){
         checkError(data);
      },
      error: function(jqXHR, textStatus, errorThrown ){
         ajax_error( jqXHR, textStatus, errorThrown );
      }
   });
}


function socketio_available(s) { // socket io est chargé, on se connecte

   $("#wait").hide();
   $( "#tabs1" ).tabs();
   $("#available").show();

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

   $("#tabs1").tabs({activate:tabsActive}); // detection des activations d'onglets.

   s.on('log', function(message){
      toLogConsole(message);
   });

   s.on('mon', function(message){
      var data = jQuery.parseJSON( message );
      anim_status(data);
   });

   s.on('rel', function(message){ // reload
      $("#table_interfaces").empty();
      $("#table_interfaces").append("<tbody></tbody>");
      load_interfaces_list_only(isadmin);
   });

   load_all_processes_lists(isadmin);
}


function unavailableMessage()
{
   var message="";
   
   message="<div>"+
           "Erreur : fonctionnalité indisponible. Consultez les messages d'erreur du serveur."+
           "</div>";
   
   return message;
}

function socketio_unavailable() {
   $("#wait").hide();
   $("#unavailable").show();
}


var _intervalId;
var _intervalCounter;
function start_index_controller()
{
   authdata=get_auth_data();
   if(authdata==false) {
      mea_alert2(str_Error+str_double_dot, str_not_connected, function(){window.location = "login.php";} );
      return false;
   } else {
      if(authdata.profil!=1) {
         isadmin=0;
      } else {
         isadmin=1;
      }
   }

   function wait_socketio_available()
   {
      _intervalCounter=0;
      _intervalId=setInterval(function() {
         var s=liveCom.getSocketio();
         console.log(typeof(s));
         if(s!==null) {
            window.clearInterval(_intervalId);
            socketio_available(s);
         }
         else {
            _intervalCounter++;
            if(_intervalCounter>25) { // 2,5 secondes max pour s'initialiser (50 x 100 ms)
               window.clearInterval(_intervalId);
               socketio_unavailable();
            }
         }
      },
      100);
   }

   _intervalCounter=0;
   _intervalId=setInterval(function() {
      if(typeof(liveCom) != "undefined") {
         console.log(typeof(liveCom));
         window.clearInterval(_intervalId);
         wait_socketio_available();
      }
      else {
         _intervalCounter++;
         if(_intervalCounter>25) { // 2,5 secondes max pour s'initialiser
            window.clearInterval(_intervalId);
            socketio_unavailable();
         }
      }
   },
   100);
}


