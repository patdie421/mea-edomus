// objet liveCom à mettre dans un .js
var liveCom = {
   socketio: null,
   _socketio_port: -1,
   
   getSocketio: function() {
      return this.socketio;
   },
   
   connect: function(port) {
      this._socketio_port=port;
      var socketioJsUrl=window.location.protocol + '//' + window.location.hostname + ':'+this._socketio_port+'/socket.io/socket.io.js';
      $.ajax({
         url: socketioJsUrl,
         dataType: "script",
         timeout: 5000,
         success: this._socketio_available,
         error: this._socketio_unavailable
      });
   },
   
   _socketio_available: function(data, textStatus, jqXHR)
   {
      if(liveCom._socketio_port<0)
         liveCom._socketio_port=8000; // port par defaut
      var socketioAddr=window.location.protocol + '//' + window.location.hostname + ':'+liveCom._socketio_port;
         liveCom.socketio = io.connect(socketioAddr);
      if(null !== liveCom.socketio)
      {
        liveCom.socketio.on('not', liveCom._notify);
      }
   },
   
   _socketio_unavailable: function(xhr, textStatus, thrownError)
   {
      xhr.abort();
      // alert("responseText="+xhr.responseText+" status="+xhr.status+" thrownError="+thrownError);
      this.socketio=null;
   },
   
   _notify: function(data)
   {
      var type="";
      var _type = data.substring(0, 1);
      var msg = data.slice(2);
      switch(_type)
      {
         // alert - success - error - warning - information - confirmation
         case "A" : type="alert"; break;
         case "E" : type="error"; break;
         case "S" : type="success"; break;
         case "W" : type="warning"; break;
         case "I" : type="information"; break;
         case "C" : type="confirmation"; break;
         default  : type="information"; break;
      };
      liveCom.notify(msg, type);
   },
   
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
            "timeout":"3000",
            "closable":true,
            "closeOnSelfClick":true
         });
      }
   }
};

$.noty.defaults.maxVisible=15;

