//
// pour lire la ligne de commande :
// process.argv.forEach(function (val, index, array) {
//  console.log(index + ': ' + val);
//});
// passer les ports et sessions_path
//


// pour la gestion de l'habilitation
//io.set('authorization', function (handshake, callback) {
//
// // if (handshake.headers.cookie) {
// //
// //   handshake.cookie = cookie.parse(handshake.headers.cookie);
// //
// //   handshake.sessionID = connect.utils.parseSignedCookie(handshake.cookie['express.sid'], 'secret');
// //
// //   if (handshake.cookie['express.sid'] == handshake.sessionID) {
// //     return callback('Cookie is invalid.', false);
// //   }
// //
// // } else {
// //   return callback('No cookie transmitted.', false);
// // }
//
//  callback(null, true);
//});

// voir ici : http://stackoverflow.com/questions/6502031/authenticate-user-for-socket-io-nodejs pour récupérer l'id session PHP dans les cookies

// pour récupérer les info de session
//function unserialize_session(str){
//    var sessHash, sessHashEnd, sess = {}, serial = '', i =0;
//    do {
//        sessHash = str.match(/(^|;)([a-zA-Z0-9_-]+)\|/i);
//        if (sessHash) {
//            str = str.substring(sessHash[0].length);
//            serial = str;
//            sessHashEnd = serial.match(/(^|;)([a-zA-Z0-9_-]+)\|/i);
//
//            if (sessHashEnd && sessHashEnd[2] && sessHashEnd[2].length > 0) {
//                serial = serial.substring(serial.search(new RegExp('(^|;)('+sessHashEnd[2]+')\\|')),0);
//                str = str.substring(str.search(new RegExp('(^|;)('+sessHashEnd[2]+')\\|')))
//            }
//
//            sess[sessHash[2]] = unserialize(serial);
//        }
//        if (i++ > 50 ) break;
//    } while (sessHash);
//
//    return sess;
//}

// pour lire un fichier (session php par exemple) voir :
// http://docs.nodejitsu.com/articles/file-system/how-to-read-files-in-nodejs


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

io.set('authorization', function (handshake, callback) {
   return callback(null, true);
   
   // retourner false si pas authentifié
});


io.sockets.on('connection', function(socket) {
   clients[socket.id] = [];
   clients[socket.id]['socket'] = socket;
   clients[socket.id]['connected'] = 0;


   clients[socket.id]['connected'] = 1; // pour les tests

   var address = socket.handshake.address;
   console.log("Connexion d'un client " + socket.id + " from : "+ address.address);

   socket.on('disconnect', function() {
      console.log("Deconnexion d'un client");
      delete clients[socket.id];
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
