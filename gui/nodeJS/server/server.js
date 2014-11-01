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
         var keyval=variables[i].split('|');
         var key=keyval[0];
         var typevalue=keyval[1].split(':');
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

// valeurs par defaut
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


function process_msg(cmnd, msg)
{
   if(cmnd=="LOG")
      sendMessage('log', msg);
   else if(cmnd=="MON")
   {
      sendMessage('mon', msg);
   }
   else if(cmnd=="NOT")
   {
      sendMessage('not', msg);
   }
   else if(cmnd=="REL")
   {
      sendMessage('rel', msg);
   }
   else
   {
      console.log("INFO   socket.on(data) : unknown command - "+cmd);
      return -1;
   }
   return 0;
}


var server = require('net').createServer(function (socket) {

   socket.on('readable',function() {
      // format d'une trame :
      // $$$%c%c%3s:%s###
      // avec %c%c : taille de la zone data ("CMD:%s") en little indian
      //      %3s  : code commande (ex : LOG, MON, ...)
      try {
         var car = socket.read(1);
         do
         {
            // recherche un debut de trame ($$$)
            var counter=0;
            var start_found=0;

            while (car)
            {
               if(car == '$')
                  counter++;
               else
                  counter=0;
               if(counter==3)
               {
                  start_found=1;
                  break;
               }
               car = socket.read(1); // lecture caractère suivant
            }
            if(start_found!=1) // pas de début de trame trouvé et plus de caractère à lire
               break; // sortie de la boucle

            // lecture de la taille de la zone de données données
            var l_data=-1;
            var size=socket.read(2); // taille sur deux octets
            if(size)
            {
               l_data=size.toString().charCodeAt(0)+size.toString().charCodeAt(1)*128;
            }
            else
            {
               continue;
            }
           
            // lecture des données
            var data = socket.read(l_data);
            if(!data)
               continue;

            // lire fin de trame : ###
            var end = socket.read(3);
            if( end.toString() != "###")
            {
               continue;
            }
            var t = data.toString();
            var cmnd = t.substring(0, 3);
            var msg = t.slice(4);

            process_msg(cmnd, msg);
         }
         while( null !== (car = socket.read(1)) ); // encore des caractères à lire ?
      }
      catch(err)
      {
         console.log("ERROR : socket.on('readable',function(){})");
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
