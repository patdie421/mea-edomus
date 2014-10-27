var whoIsScrollingFlag=0; // 1 = le script, 0 = l'utilisateur
var scrollOnOffFlag=1;    // 1 = activé par defaut


// objet liveCom à mettre dans un .js
var liveCom = {
   socketio: null;
   _socketio_port = -1;
   getSocketio: function() {
      return this.socketio;
   };
   connect: function(port) {
      this._socketio_port=port;
      var socketioJsUrl=window.location.protocol + '//' + window.location.hostname + ':'+_socketio_port+'/socket.io/socket.io.js';
      $.ajax({
         url: socketioJsUrl,
         dataType: "script",
         timeout: 5000,
         success: this._socketio_available,
         error: this._socketio_unavailable
      });
   };
   _socketio_available: function ()
   {
      if(this._socketio_port<0)
         this._socketio_port=8000; // port par defaut
      var socketioAddr=window.location.protocol + '//' + window.location.hostname + ':'+socketio_port;
         this.socketio = io.connect(socketioAddr);
      if(null !== this.socketio)
      {
        this.socketio.on('not', this._notify);
      }
   };
   _socketio_unavailable: function(xhr, textStatus, thrownError)
   {
      jqXHR.abort();
      alert("responseText="+xhr.responseText+" status="+xhr.status+" thrownError="+thrownError);
      this.socketio=null;
   };
   _notify: function(data)
   {
      var type="";
      var _type = data.substring(0, 0);
      var msg = data.slice(2);
      switch(_type)
      {
         // alert - success - error - warning - information - confirmation
         case "A" : type="alert"; break;
         case "S" : type="success"; break;
         case "W" : type="warning"; break;
         case "I" : type="information"; break;
         case "C" : type="confirmation"; break;
         default  : type="information"; break;
      };
      this.notify(msg, type);
   };
   notify: function(msg, type)
   {
      if(typeof noty != "undefined")
      {
         noty({
            "text":msg,
            "layout":"topRight",
            "type":type,
            "textAlign":"center",
            "easing":"swing",
            "animateOpen":{"height":"toggle"},
            "animateClose":{"height":"toggle"},
            "speed":"500",
            "timeout":"2000",
            "closable":true,
            "closeOnSelfClick":true
         });
      }
   }
}


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


function anim_status(data)
{
   for(var key in data)
   {
      if(data[key]['status']==1)
      {
         if(data[key]['heartbeat']=='KO')
         {
            $("#process_"+data[key]['pid']).css("background","black");
         }
         else
         {
            $("#process_"+data[key]['pid']).css("background","green");
         }
      }
      else if(data[key]['status']==0 && data[key]['type']!=3)
      {
         $("#process_"+data[key]['pid']).css("background","red");
      }
      else
      {
         $("#process_"+data[key]['pid']).css("background","gray");
      }
   }
}


function add_row(table, name, id, start_str, start, stop_str, stop)
{
   newRow =  "<tr>" +
                 "<td style=\"width:70px;\"><div id=\"process_"+id+"\" class=\"pastille ui-widget ui-widget-content ui-corner-all\" style=\"background:gray;\"></div></td>" +
                 "<td style=\"width:230px;\"><div class=\"process\">"+name+"</div></td>" +
                 "<td style=\"width:200px;\">" +
                     "<div class=\"bouton\">";
   if(start != null)
   {
      newRow=newRow+    "<button id=\"bstart"+id+"\">"+start_str+"</button>";
   }
   if(stop != null)
   {
      newRow=newRow+    "<button id=\"bstop"+id+"\">"+stop_str+"</button>";
   }
   newRow=newRow+    "</div>" +
                 "</td>" +
             "</tr>";

    $("#"+table+" > tbody").before(newRow);
   $("#bstop"+id).button().click(function(event){stop(id);});
   $("#bstart"+id).button().click(function(event){start(id);});
};


function load_processes_list(){
   $.ajax({
      url: 'CMD/ps.php',
      async: true,
      type: 'GET',
      dataType: 'json',
      success: function(data){
         for(var key in data)
         {
            if(data[key]['type']==2)
               continue;
            if(data[key]['group']==0)
            {
               add_row("table_processes", key, data[key]['pid'], "start", start, "stop", stop);
            }
            else if(data[key]['group']==2)
            {
               add_row("table_reload", key, data[key]['pid'], "reload", start, null, null);
            }
            else if(data[key]['group']==1)
            {
               add_row("table_interfaces", key, data[key]['pid'], "start", start, "stop", stop);
            }
         }
         anim_status(data);
      },
      error: function(jqXHR, textStatus, errorThrown ){
         ajax_error( jqXHR, textStatus, errorThrown );
      }
   });
}


function socketio_available(s) { // socket io est chargé, on se connecte
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


   s.on('log', function(message){
      toLogConsole(message);
   });

   s.on('mon', function(message){
      var data = jQuery.parseJSON( message );
      anim_status(data);
   });
   
   load_processes_list();
}


function socketio_unavailable() {
   $("#console").append("<div>Pas d'iosocket => pas d'info en live ...<div>");
}


function start(id)
{
   $.ajax({
      url: 'CMD/startstop.php?process='+id+'&cmnd=start',
      async: true,
      type: 'GET',
      dataType: 'json',
      success: function(data){
         var type="";
         if(data["errno"]==0)
            type="success";
         else if(data["errno"]==-1)
            type="information";
         else
            type="error";

         liveCom.notify(data["errmsg"],type);
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
         var type="";
         if(data["errno"]==0)
            type="success";
         else if(data["errno"]==-1)
            type="information";
         else
            type="error";

         liveCom.notify(data["errmsg"],type);
      },
      error: function(jqXHR, textStatus, errorThrown ){
         ajax_error( jqXHR, textStatus, errorThrown );
      }
   });
}


function start_index_controller(port)
{
  s=liveCom.getSocketio();
  if(null !== s)
  {
     socketio_available(s);
  }
  else
  {
     socketio_unavailable();
  }
}

