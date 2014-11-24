var isadmin=-1;


function socketio_available(s) { // socket io est chargé, on se connecte
   
   s.on('mon', function(message){
      var data = jQuery.parseJSON( message );

      try {
         controlPanel.update_status(data);
      }
      catch(ex) {
      }

      try {
         indicatorsTable.update_all_indicators(data);
      }
      catch(ex) {
      }
   });

   s.on('rel', function(message){
      try {
         controlPanel.load_interfaces_list_only(isadmin);
      }
      catch(ex) {
      }

      try {
         indicatorsTable.reload("table_indicateurs");
      }
      catch(ex) {
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
      $.messager.alert(str_Error+str_double_dot,str_not_connected,'error', function(){window.location = "login.php";});
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

