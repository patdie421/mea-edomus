var isadmin=1;


function ajax_error(xhr, ajaxOptions, thrownError){
    alert("index.js : responseText="+xhr.responseText+" status="+xhr.status+" thrownError="+thrownError);
}


var indicatorsTable = {
   compteur: 0,
   indicatorsToExclude: ["desc","pid","status","type","group","heartbeat"],
   add_process: function (table, name)
   {
      newRow = "<tr>" +
                   "<td class=\"processes_section\"><div class=\"processes_name\">"+name+"</div><div id=\"desc_name\" class=\"processes_description\"></div></td>" +
                   "<td class=\"indicators_section\"><div>" +
                      "<table class=\"table_indicators\" id=\""+name+"\">" +
                      "</table>" +
                   "</div></td>" +
               "</tr>";
      $("#"+table).append(newRow);
   },

   add_indicator: function (process, name, indicator_name)
   {
      var couleur;
      if(this.compteur % 2 == 0)
          couleur=1;
       else
          couleur=2;

      newRow = "<tr class=\"couleur_"+couleur+"\">" +
                  "<td class=\"indicator_name\"><div>"+name+"</div></td>"+
                  "<td class=\"indicator_value\"><div id=\""+process+"_"+indicator_name+"\"\">N/A</div></td>" +
               "</tr>";
      $("#"+process).append(newRow);
      this.compteur++;
   },

   build: function(table, processes)
   {
      var nb=0;
      for(var process in processes)
      {
         if(processes[process].length>0)
          {
             this.add_process(table, process);
             for(var indicator in processes[process])
             {
                this.add_indicator(process, processes[process][indicator], processes[process][indicator]);
                nb++;
             }
         }
      }
      return nb;
   },
   
   load: function(table)
   {
      $.ajax({
         url: 'CMD/indicators.php',
         async: true,
         type: 'GET',
         dataType: 'json',
         success: function(data){
            $("#"+table).empty();
            var ret=this.build(table, data);
            if(ret<=0)
            {
               console.log("erreur");
               // mettre un not available ...
            }
         },
         error: function(jqXHR, textStatus, errorThrown ){
            ajax_error( jqXHR, textStatus, errorThrown );
         }
      });
   }

   update_indicator: function (process, indicator, value)
   {
      $("#"+process+"_"+indicator).text(value);
   },
   
   update_all_indicators: function(data)
   {
      for(process in data)
      {
          for(indicator in data[process])
          {
              if(this.indicatorsToExclude.indexOf(indicator) == -1)
                 indicatorsTable.update_indicator(process, indicator, data[process][indicator]);
          }
      }
   }
}  


var logViewer = {

   whoIsScrollingFlag:0; // 1 = le script, 0 = l'utilisateur
   scrollOnOffFlag:1;    // 1 = activé par defaut
  console:"",
   append_line: function(line)
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
      $("#"+this.console).append("<div class='log' style='white-space:pre-wrap; color:"+color+";'>"+line+"</div>");
     
      // on retire une ligne si la limite est atteinte
      var logLineDivHigh = 0;
      if($('#'+this.console+" > *').length >= 1000)
      {
         var toRemove = $("#"+this.console+" div:first-child"); // ligne à retirer
         this.whoIsScrollingFlag=1;
         logLineDivHigh = toRemove.height(); // hauteur de la ligne à retirer
         toRemove.remove(); // on la retire
      }

      if(this.scrollOnOffFlag==1) { // le scroll est actif
         this.whoIsScrollingFlag=1; // on dis qu'on va générer un événement de scroll (il faudra ne pas en tenir compte)
         $("#"+this.console).scrollTop($("#"+this.console).prop("scrollHeight"));
      }
      else
      {
         // le scroll n'est pas actif
         if(logLineDivHigh) // si une ligne a été retirée
            $("#"+this.console).scrollTop($("#"+this.console).scrollTop()-logLineDivHigh); // je remonte le slider de la hauteur de la ligne retirée
      }
   },
   
   start: function(s, console) {
      this.console=console;
      var _logViewer = this;
      $("#"+this.console).scroll(function() {
         if(_logViewer.scrollOnOffFlag==0 && _logViewer.whoIsScrollingFlag==0) // si scroll inactif
         {
            // le slider a-t-il été poussé jusqu'en bas ?
            if($(this).scrollTop() + $(this).innerHeight() >= $(this).scrollHeight) {
               _logViewer.scrollOnOffFlag=1; // on réactive le scroll (live)
            }
         }
         else if(_logViewer.whoIsScrollingFlag==0) // le scroll est activé et c'est l'utilisateur qui à scroller
         {
            _logViewer.scrollOnOffFlag=0; // dans ce cas on désactive le scroll
         }
         else
            _logViewer.whoIsScrollingFlag=0; // remise à 0 du flag.
      });

      s.on('log', function(message){
         _logViewer.append_line(message);
      });
   },
   
   scrollBottom: function()
   {
      this.scrollOnOffFlag=1;
      this.whoIsScrollingFlag=1;

      // on se met en bas de la log
      $("#"+this.console).scrollTop($("#"+this.console).height()); // on scroll à la fin de la div

   }
}
    

var controlPanel = {
   table_reload:"",
   table_interfaces:"",
   table_processes:"",
   init: function(table_reload, table_interfaces, table_processes)
   {
      this.table_reload="#"+table_reload;
      this.table_interfaces="#"+table_interfaces;
      this.table_processes="#"+table_processes;   
   },
   
   update_status: function(data)
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
   },
   
   add_row: function(name, desc, id, start_str, start, stop_str, stop, isadmin)
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

      $("#"+this.table+" > tbody").before(newRow);
      $("#bstop"+id).button().click(function(event){stop(id);});
      $("#bstart"+id).button().click(function(event){start(id);});
   },

   messageListeInterfaceVide: function(table)
   {
      var newRow="";
      newRow = "<tr>" +
                  "<td style=\"width:100%; height:40px; text-align:center;\"><div>Aucune interface disponible</div></td>" +
               "</tr>";
      $("#"+table > tbody").before(newRow);
   },

   load: function(isadmin) {
      _controlPanel=this;
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
                  _controlPanel.add_row(_controlPanel.table_processes, key, data[key]['desc'], data[key]['pid'], "start", start, "stop", stop, isadmin);
               }
               else if(data[key]['group']==2)
               {
                  _controlPanel.add_row(_controlPanel.table_reload, key, data[key]['desc'], data[key]['pid'], "reload", reload, null, null, isadmin);
               }
               else if(data[key]['group']==1)
               {
                  nb++;
                  _controlPanel.add_row(_controlPanel.table_interfaces, key, data[key]['desc'], data[key]['pid'], "start", start, "stop", stop, isadmin);
               }
            }
            if(nb==0)
               _controlPanel.messageListeInterfaceVide(_controlPanel.table_interfaces);
            _controlPanel.update_status(data);
         },
         error: function(jqXHR, textStatus, errorThrown ){
            ajax_error( jqXHR, textStatus, errorThrown );
         }
      });
   },
   
   load_interfaces_list_only: function(isadmin) {
      _controlPanel=this;
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
                  _controlPanel.add_row(_controlPanel.table_interfaces, key, data[key]['desc'], data[key]['pid'], "start", start, "stop", stop, isadmin);
                  nb++;
               }
            }
            if(nb>0)
               _controlPanel.update_status(data);
            else
               _controlPanel.messageListeInterfaceVide(_controlPanel.table_interfaces);
         },
         error: function(jqXHR, textStatus, errorThrown ){
            ajax_error( jqXHR, textStatus, errorThrown );
         }
      });
   },
   
   reloadTask function(id)
   {
      _controlPanel=this;

      $(_controlPanel.table_interfaces).empty();
      $(_controlPanel.table_interfaces).append("<tbody></tbody>");
      $(_controlPanel.table_interfaces+" > tbody").before("<tr><td style=\"width:100%; margin:auto;\" align=\"center\" valign=\"top\"><div class=\"wait_ball\"></div></td></tr>");
      $.ajax({
         url: 'CMD/startstop.php?process='+id+'&cmnd=task',
         async: true,
         type: 'GET',
         dataType: 'json',
         success: function(data){
            if(_controlPanel.checkError(data)==false)
            {
               // on enleve juste la roue qui tourne ...
               $(_controlPanel.table_interfaces).empty();
               $(_controlPanel.table_interfaces).append("<tbody></tbody>");
            }
         },
         error: function(jqXHR, textStatus, errorThrown ){
            $(_controlPanel.table_interfaces).empty();
            $(_controlPanel.table_interfaces).append("<tbody></tbody>");
            ajax_error( jqXHR, textStatus, errorThrown );
         }
      });
   },
   
   function startTask(id)
   {
      _controlPanel=this;
      $.ajax({
         url: 'CMD/startstop.php?process='+id+'&cmnd=start',
         async: true,
         type: 'GET',
         dataType: 'json',
         success: function(data){
            _controlPanel.checkError(data);
         },
         error: function(jqXHR, textStatus, errorThrown ){
            ajax_error( jqXHR, textStatus, errorThrown );
         }
      });
   },
   
   stopTask function(id)
   {
      _controlPanel=this;
      $.ajax({
         url: 'CMD/startstop.php?process='+id+'&cmnd=stop',
         async: true,
         type: 'GET',
         dataType: 'json',
         success: function(data){
            _controlPanel.checkError(data);
         },
         error: function(jqXHR, textStatus, errorThrown ){
            ajax_error( jqXHR, textStatus, errorThrown );
         }
      });
   },
   
   checkError function(data)
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
}


function tabsActive(event,ui) {
   active = $("#tabs1").tabs("option", "active");
   if(active == 2) // activation de la console
   {
      logViewer.scrollBottom();
   }
}


function socketio_available(s) { // socket io est chargé, on se connecte

   $("#wait").hide();
   $( "#tabs1" ).tabs();
   $("#available").show();
   $("#tabs1").tabs({activate:tabsActive}); // detection des activations d'onglets.
   controlPanel.init("table_reload", "table_interfaces", "table_processes");
   controlPanel.load(isadmin);
   indicatorsTable.load("table_indicateurs");
   logViewer.start(s, "console");

   s.on('mon', function(message){
      var data = jQuery.parseJSON( message );
      controlPanel.update_status(data);
      indicatorsTable.update_all_indicators(data);
   });

   s.on('rel', function(message){ // reload
      $("#table_interfaces").empty();
      $("#table_interfaces").append("<tbody></tbody>");
      controlPanel.load_interfaces_list_only(isadmin);
   });
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


