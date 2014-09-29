//
// pour lire la ligne de commande :
// process.argv.forEach(function (val, index, array) {
//  console.log(index + ': ' + val);
//});
// passer les ports et sessions_path
//


// voir ici : http://stackoverflow.com/questions/6502031/authenticate-user-for-socket-io-nodejs pour récupérer l'id session PHP dans les cookies

// pour récupérer les info d'un fichier php session
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


var server = require('net').createServer(function (socket) {
   socket.on('data', function (msg) {
      var t = msg.toString().split(/\r?\n/);
      for (var i in t)
      {
         console.log("Message recu : " + t[i]);
         sendMessage(i);
      }
   });
}).listen(LOCAL_PORT);

server.on('listening', function () {
   console.log("Server accepting connection on port: " + LOCAL_PORT);
});


var clients = [];

var io = require('socket.io').listen(SOKET_IO_PORT);

/*
io.set('authorization', function (handshake, callback) {
   return callback(null, true);
   
   // retourner false si pas authentifié
});
*/

// déclaration d'une fonction qui sera activé lors de la connexion. On traite ici l'authorisation on non (à voir comment)
io.use(function(socket, next) {
  var handshakeData = socket.request;

//  var cookies=handshakeData.headers.['cookie'];
  var cookies=handshakeData.headers.cookie;

  console.log(cookies); // pour voir ce qu'il y a dans les cookies à la connexion
//  console.log(cookies.PHPSESSID); // voir si c'est possible

  // make sure the handshake data looks good as before
  // if error do this:
  //    next(new Error('not authorized');
  // else just call next

  // next(new Error('not authorized'); // connexion refusée
  next(); // connexion authorisée
});


io.sockets.on('connection', function(socket) {
   clients[socket.id] = [];
   clients[socket.id]['socket'] = socket;

   var address = socket.handshake.address;
   console.log("Connexion d'un client " + socket.id + " depuis : "+ address.address);

   socket.on('disconnect', function() {
      console.log("Deconnexion du client : "+addsocket.id); 
      delete clients[socket.id];
  });
});


function sendMessage(message) {
   
   // emission du message à tous les clients "authentifies"
   for (var i in clients) {
      clients[i].socket.emit('log', message.toString());
      console.log(message+" emis vers : "+i);
   }
}
