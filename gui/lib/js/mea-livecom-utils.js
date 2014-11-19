var liveCom = {
   socketio: null,
   _socketio_port: -1,
   
   getSocketio: function() {
      return this.socketio;
   },
   
   connect: function(port)
   {
      this._connect(port, this);
   },
   
   _connect: function(port,_liveCom) {
     _liveCom._socketio_port=port;
       var socketioJsUrl=window.location.protocol + '//' + window.location.hostname + ':'+_liveCom._socketio_port+'/socket.io/socket.io.js';
      $.ajax({
         url: socketioJsUrl,
         dataType: "script",
         timeout: 5000,
         success: function(data, textStatus, jqXHR) // _socket_available
                  {
                     var socketioAddr=window.location.protocol + '//' + window.location.hostname + ':'+_liveCom._socketio_port;
                     _liveCom.socketio = io.connect(socketioAddr);
                     if(null !== _liveCom.socketio)
                     {
                        _liveCom.socketio.on('not', function(data)
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
                           _liveCom.notify(msg, type);
                        });
                     }
                  },
         error:   function(xhr, textStatus, thrownError)
                  {
                     xhr.abort();
                     // alert("responseText="+xhr.responseText+" status="+xhr.status+" thrownError="+thrownError);
                     _liveCom.socketio=null;
                     _liveCom._socketio_port=-1;
                  }
      });
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

