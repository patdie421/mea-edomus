var LOCAL_PORT = 5600;
var SOKET_IO_PORT = 8000;

var key="12345678";

var server = require('net').createServer(function (socket) {
   socket.on('data', function (msg) {
      console.log("Message recu : " + msg);
      sendMessage(msg);
   });
}).listen(LOCAL_PORT);

server.on('listening', function () {
   console.log("Server accepting connection on port: " + LOCAL_PORT);
});


var clients = [];

var io = require('socket.io').listen(SOKET_IO_PORT);

io.sockets.on('connection', function(socket) {
   clients[socket.id] = [];
   clients[socket.id]['socket'] = socket;
   clients[socket.id]['connected'] = 0;
   socket.emit('key', '?');

   var address = socket.handshake.address;
//    console.log("New connection from " + address.address + ":" + address.port);
   console.log("Connexion d'un client " + socket.id + " from : "+ address.address);

   socket.on('disconnect', function() {
      console.log("Deconnexion d'un client");
      delete clients[socket.id];
  });

  socket.on('key', function(ckey) {
     console.log("cle recu de "+this.id);
     if(ckey == key)
     {
        console.log("Comparaison des cles OK : "+ckey+" == "+key);
        clients[this.id]['connected']=1;
     }
     else
     {
        console.log("Comparaison des cles KO : "+ckey+" != "+key);
     }
  });
});


function sendMessage(message) {
   // emission du message à tous les clients "authentifies"
   for (var i in clients) {
      if(clients[i]['connected'] == 1)
      {
         clients[i].socket.emit('log', message.toString());
         console.log(message+" emis vers : "+i);
      }
      else
         console.log(i+" pas connecte");
   }
}
