//
// pour lire la ligne de commande :
// process.argv.forEach(function (val, index, array) {
//  console.log(index + ': ' + val);
//});
// passer les ports et sessions_path
//


// voir ici : http://stackoverflow.com/questions/6502031/authenticate-user-for-socket-io-nodejs pour récupérer l'id session PHP dans les cookies


function unserialize_session(str) { // c'est tres tres light mais suffisant pour l'appli ...
   unserialized={};
   
   var variables=str.split(';');
   for(i=0;i<variables.length;i++)
   {
      if(variables[i])
      {
         keyval=variables[i].split('|');
         key=keyval[0];
         typevalue=keyval[1].split(':');
         if(typevalue[0]=='s')
         {
            unserialized[key]=typevalue[2].substring(1,typevalue[2].length-1);
         }
         else if(typevalue[0]=='i')
         {
            unserialized[key]=typevalue[1];
         }
      }
   }
   return unserialized;
}


var LOCAL_PORT = 5600;
var SOKET_IO_PORT = 8000;
var PHPSESSION_PATH = "/tmp";

// récupération des paramètres de la ligne de commande 
for (var i = 2; i < process.argv.length; i++) {
   var opt=process.argv[i].split('=');
   
   if(opt[0]=="--iosocketport")
   try {
      SOKET_IO_PORT=Number(opt[1]);
   } catch(e) {
      console.log("not a number");
   }
   else if(opt[0]=="--dataport")
   {
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
         var cmd = t[i].substring(0, 3);
         var msg = t[i].slice(4);
         if(cmd=="LOG")
            sendMessage('log', msg);
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

   var cookiesStr=handshakeData.headers.cookie;
   cookies = {}
   cookies_splited=cookiesStr.split(';');
   for(i=0;i<cookies_splited.length;i++)
   {
      parts = cookies_splited[i].split('=');
      cookies[parts[0].trim()] = (parts[1] || '').trim()
   }

   var phpSessionFile = PHPSESSION_PATH+"/sess_"+cookies['PHPSESSID'];

   try {
      data = fs.readFileSync(phpSessionFile,'utf8');
      console.log("INFO  fs.readFile("+phpSessionFile+") = "+data);
      sess={};
   }
   catch(e) {
      console.log("ERROR fs.readFileSync("+phpSessionFile+") : "+e.message);
      next(new Error('not authorized'));
      return;
   }

   try {
      sess=unserialize_session(data.toString());
   }
   catch(e) {
      console.log("ERROR unserialize_session() : "+e.message);
      next(new Error('not authorized'));
      return;
   }

   if(sess['logged_in']==1) {
      console.log("INFO  io.use() : authorized");
      next();
      return;
   }
   else
   {
      console.log("INFO  io.use() : not authorized");
      next(new Error('not authorized'));
      return;
   }
});


io.sockets.on('connection', function(socket) {
   clients[socket.id] = [];
   clients[socket.id]['socket'] = socket;

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
