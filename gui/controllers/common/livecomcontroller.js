LiveComController = function(port)
{
   this.socketio=null;
   this._socketio_port=port;
   
   $.noty.defaults.maxVisible=15;
}


LiveComController.prototype = {
   getSocketio: function() {
      return this.socketio;
   },
   
   start: function(fn_ok,fn_ko)
   {
      this._connect(this, fn_ok, fn_ko);
   },
   
   connect: function()
   {
      this._connect(this, null, null);
   },
   
   _connect: function(_liveCom,fn_ok,fn_ko) {
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
                        _liveCom.socketio.removeAllListeners('not');
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
                       
                        if(fn_ok)
                           fn_ok(_liveCom.socketio);
                     }
                  },
         error:   function(xhr, textStatus, thrownError)
                  {
                     xhr.abort();
                     $.messager.show({
                         title: _controller._toLocalC('error'),
                         msg: _controller._toLocalC("communication error")+' ('+textStatus+')'
                     });
                     _liveCom.socketio=null;
                     _liveCom._socketio_port=-1;
                     if(fn_ko)
                        fn_ko();
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

