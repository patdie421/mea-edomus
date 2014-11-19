var isadmin=-1;


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

