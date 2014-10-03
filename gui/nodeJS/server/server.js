//
// pour lire la ligne de commande :
// process.argv.forEach(function (val, index, array) {
//  console.log(index + ': ' + val);
//});
// passer les ports et sessions_path
//


// voir ici : http://stackoverflow.com/questions/6502031/authenticate-user-for-socket-io-nodejs pour récupérer l'id session PHP dans les cookies

// pour récupérer les info d'un fichier php session
function unserialize_session(str){
   var sessHash, sessHashEnd, sess = {}, serial = '', i =0;
   do {
      sessHash = str.match(/(^|;)([a-zA-Z0-9_-]+)\|/i);
      if (sessHash) {
         str = str.substring(sessHash[0].length);
         serial = str;
         sessHashEnd = serial.match(/(^|;)([a-zA-Z0-9_-]+)\|/i);

         if (sessHashEnd && sessHashEnd[2] && sessHashEnd[2].length > 0) {
            serial = serial.substring(serial.search(new RegExp('(^|;)('+sessHashEnd[2]+')\\|')),0);
            str = str.substring(str.search(new RegExp('(^|;)('+sessHashEnd[2]+')\\|')))
         }

         sess[sessHash[2]] = unserialize(serial);
      }
      if (i++ > 50 ) break;
   } while (sessHash);

   return sess;
}


var LOCAL_PORT = 5600;
var SOKET_IO_PORT = 8000;
var PHPSESSION_PATH = "/tmp";

// récupération des paramètres de la ligne de commande 
for (var i = 2; i < process.argv.length; i++) {
   var opt=process.argv[i].split(';');
   if(opt[0]=="--iosocketport")
   try {
      SOKET_IO_PORT=Number(opt[1]);
   } catch(e) {
      console.log("not a number");
   }
   else if(opt[0]=="--dataport")
      try {
         LOCAL_PORT=Number(opt[1]);
      } catch(e) {
         console.log("not a number");
      }
   }
   else if(opt[0]=="--phpsession_path")
   {
      PHPSESSION_PATH=opt[1];
   }
   else
   {
      console.log("WARNING unknow parameter "+opt[0]);
   }
}


var server = require('net').createServer(function (socket) {
   socket.on('data', function (msg) {
      var t = msg.toString().split(/\r?\n/);
      for ( i=0; i<t.length-1; i++)
      {
         sendMessage('log', t[i].slice(4));
      }
   });
}).listen(LOCAL_PORT);


server.on('listening', function () {
   console.log("INFO  server.on(listening) : Server accepting connection on port: " + LOCAL_PORT);
});


var clients = [];
var io = require('socket.io').listen(SOKET_IO_PORT);
var fs = require('fs');

// déclaration d'une fonction qui sera activée lors de la connexion (à vérifier). On traite ici l'authorisation ou non
io.use(function(socket, next) {
   var handshakeData = socket.request;

   try {
      if(clients[socket.id][logged_in]==1) // déjà connecté, pas la peine de perdre du temps
      {
         next();
         return;
      }
   }

   var cookiesStr=handshakeData.headers.cookie;
   cookies = {}
   cookiesStr.split(';').forEach (cookie) ->
      parts = cookie.split '='
      cookies[parts[0].trim()] = (parts[1] || '').trim()
      return

   var phpSessionFile = PHPSESSION_PATH+"sess_"+cookies['PHPSESSID'];

   try {
      data = fs.readFileSync(phpSessionFile,'utf8');
      console.log("INFO  fs.readFile("+phpSessionFile+") = "+data);
      sess={};
      sess=unserialize_session(data);
      if(sess['logged_in']==1) {
         console.log("INFO  io.use() : authorized");
         clients[socket.id]['logged_in']=1;
         next();
         return;
      else
         console.log("INFO  io.use() : not authorized");
         next(new Error('not authorized'));
         return;
      } catch(e) {
         console.log("ERROR fs.readFile("+phpSessionFile+") : \n"+e.message);
         next(new Error('not authorized'));
         return;
      }
   }
});


io.sockets.on('connection', function(socket) {
   clients[socket.id] = [];
   clients[socket.id]['socket'] = socket;
   clients[socket.id]['logged_in'] = 0;

   var address = socket.handshake.address;
   console.log("INFO  io.sockets.on('connection') : new client : " + socket.id + " from "+ address.address);

   socket.on('disconnect', function() {
      console.log("INFO  socket.on('disconnect') : client "+socket.id+" disconnected"); 
      delete clients[socket.id];
  });
});


function sendMessage(key, message) {
   // emission du message 'key' à tous les clients "authentifies"
   for (var i in clients) {
      clients[i].socket.emit(key, message.toString());
   }
}
