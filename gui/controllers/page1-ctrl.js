var isadmin=-1;

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
                  _controlPanel.add_row(_controlPanel.table_processes, key, data[key]['desc'], data[key]['pid'], "start", _controlPanel.startTask, "stop", _controlPanel.stopTask, isadmin);
               }
               else if(data[key]['group']==2)
               {
                  _controlPanel.add_row(_controlPanel.table_reload, key, data[key]['desc'], data[key]['pid'], "reload", _controlPanel.reloadTask, null, null, isadmin);
               }
               else if(data[key]['group']==1)
               {
                  nb++;
                  _controlPanel.add_row(_controlPanel.table_interfaces, key, data[key]['desc'], data[key]['pid'], "start", _controlPanel.startTask, "stop", _controlPanel.stopTask, isadmin);
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


   add_row: function(table, name, desc, id, start_str, start, stop_str, stop, isadmin)
   {
      _controlPanel=this;
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

      $(table).append(newRow);
//      $(table+" > tbody").before(newRow);
//      $("#bstop"+id).button().click(function(event){stop(_controlPanel,id);});
//      $("#bstart"+id).button().click(function(event){start(_controlPanel,id);});
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
//               $("#bstart"+data[key]['pid']).button("option", "disabled", true );
//               $("#bstop"+data[key]['pid']).button( "option", "disabled", false );
            }
         }
         else if(data[key]['status']==0 && data[key]['type']!=2) // non demarré
         {
            $("#process_"+data[key]['pid']).css("background","red");
//            $("#bstart"+data[key]['pid']).button("option", "disabled", false );
//            $("#bstop"+data[key]['pid']).button( "option", "disabled", true );
         }
         else
         {
            $("#process_"+data[key]['pid']).css("background","gray");
         }
      }
   },


   messageListeInterfaceVide: function(table)
   {
      var newRow="";
      newRow = "<tr>" +
                  "<td style=\"width:100%; height:40px; text-align:center;\"><div>Aucune interface disponible</div></td>" +
               "</tr>";
      $("#"+table+" > tbody").before(newRow);
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
                  _controlPanel.add_row(_controlPanel.table_interfaces, key, data[key]['desc'], data[key]['pid'], "start", _controlPanel.startTask, "stop", _controlPanel.stopTask, isadmin);
                  nb++;
               }
            }
            if(nb>0)
               _controlPanel.update_status(data);
            else
               _controlPanel.messageListeInterfaceVide(_controlPanel.table_interfaces);
         },
         error: function(jqXHR, textStatus, errorThrown) {
            ajax_error(jqXHR, textStatus, errorThrown);
         }
      });
   },

   reloadTask: function(_controlPanel,id)
   {
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
         error: function(jqXHR, textStatus, errorThrown ) {
            $(_controlPanel.table_interfaces).empty();
            $(_controlPanel.table_interfaces).append("<tbody></tbody>");
            ajax_error( jqXHR, textStatus, errorThrown );
         }
      });
   },
   
   startTask: function(_controlPanel,id)
   {
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
   
   stopTask: function(_controlPanel,id)
   {
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
   
   checkError: function(data)
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
};


function socketio_available(s) { // socket io est chargé, on se connecte
//   controlPanel.init("table_reload", "table_interfaces", "table_processes");
//   controlPanel.load(isadmin);

   s.on('mon', function(message){
      var data = jQuery.parseJSON( message );
      if(controlPanel !== undefined && controlPanel.isStarted())
      {
         controlPanel.update_status(data);
      }
      if(indicatorsTable !== undefined && indicatorsTable.isStarted())
      {
         indicatorsTable.update_all_indicators(data);
      }
   });

   s.on('rel', function(message){ // reload
      $("#table_interfaces").empty();
      $("#table_interfaces").append("<tbody></tbody>");
      if(controlPanel !== undefined && controlPanel.isStarted())
      {
         controlPanel.load_interfaces_list_only(isadmin);
      }
      if(indicatorsTable !== undefined && indicatorsTable.isStarted())
      {
         indicatorsTable.reload("table_indicateurs");
      }
   });
}


function socketio_unavailable() {
}


var _intervalId;
var _intervalCounter;
function page1_controller()
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

