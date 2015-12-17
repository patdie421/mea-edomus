//
// pour lire la ligne de commande :
// process.argv.forEach(function (val, index, array) {
//  console.log(index + ': ' + val);
//});
// passer les ports et sessions_path
//

// voir ici : http://stackoverflow.com/questions/6502031/authenticate-user-for-socket-io-nodejs pour récupérer l'id session PHP dans les cookies

function getDateTime()
{
// [2014-11-12 21:52:30]
    var date = new Date();

    var hour = date.getHours();
    hour = (hour < 10 ? "0" : "") + hour;

    var min  = date.getMinutes();
    min = (min < 10 ? "0" : "") + min;

    var sec  = date.getSeconds();
    sec = (sec < 10 ? "0" : "") + sec;

    var year = date.getFullYear();

    var month = date.getMonth() + 1;
    month = (month < 10 ? "0" : "") + month;

    var day  = date.getDate();
    day = (day < 10 ? "0" : "") + day;

    return "["+year+"-"+month+"-"+day+" "+hour+":"+min+":"+sec+"]";
}


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
      console.log(getDateTime()+" WARNING unknow parameter "+opt[0]);
   }
}


setInterval(function() {
            }, 5 * 1000);


function internalCmnd(s, msg)
{
   var cmnd = msg.substring(0, 1);
   var data = msg.slice(2);
  
   switch(cmnd)
   {
      case "S" : // demande d'arret du moteur
         console.log(getDateTime()+" INFO  : (internalCmnd) OK Chef, nodejs goes done");
         process.exit(0);
         break;
      case "P" : // envoyer un retour sur s
         s.write("OK");
         break;
      case "D"  : // déconnexion d'un client
         console.log(getDateTime()+" INFO  : send deconnection request to %s", data);
         clients[phpsessids[data]].socket.emit("dct","now");
         break;
     default:
         break;     
   }
}


function process_msg(s, cmnd, msg)
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
   else if(cmnd=="INT")
   {
      internalCmnd(s, msg);
   }
   else
   {
      console.log(getDateTime()+" INFO   socket.on(data) : unknown command - "+cmd);
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
               // voir buf.readUInt8(offset, [noAssert])# pour lire les données.
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

            process_msg(socket, cmnd, msg);
         }
         while( null !== (car = socket.read(1)) ); // encore des caractères à lire ?
     }
     catch(err)
     {
        console.log(getDateTime()+" ERROR : socket.on('readable',function(){})");
     }
   });
}).listen(LOCAL_PORT);


server.on('listening', function () {
   console.log(getDateTime()+" INFO  server.on(listening) : Server accepting connection on port: " + LOCAL_PORT);
});


var clients = [];
var phpsessids = [];

var io = require('socket.io').listen(SOKET_IO_PORT);
var fs = require('fs');

// déclaration d'une fonction qui sera activée lors de la connexion. On traite ici l'authorisation ou non
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

   var phpsessid=cookies['PHPSESSID'];
   var phpSessionFile = PHPSESSION_PATH+"/sess_"+phpsessid;

   try {
      data = fs.readFileSync(phpSessionFile,'utf8');
      console.log(getDateTime()+" INFO  fs.readFile("+phpSessionFile+") = "+data);
      sess={};
   }
   catch(e) {
      console.log(getDateTime()+" ERROR fs.readFileSync("+phpSessionFile+") : "+e.message);
      next(new Error('not authorized'));
      return;
   }

   try {
      sess=unserialize_session(data.toString());
   }
   catch(e) {
      console.log(getDateTime()+" ERROR unserialize_session() : "+e.message);
      next(new Error('not authorized'));
      return;
   }

   if(sess['logged_in']==1) {
      console.log(getDateTime()+" INFO  io.use() : authorized");
      phpsessids[phpsessid]=socket.id; // on garde trace de la session pour pouvoir deconnecter le client lorsque la session est expirer (coté serveur).
      socket.phpsessid=phpsessid;      // et dans l'autre sens pour éviter les recherches par boucle lors de la deconnexion.
      next();
      return;
   }
   else
   {
      console.log(getDateTime()+" INFO  io.use() : not authorized");
      next(new Error('not authorized'));
      return;
   }
});


io.sockets.on('connection', function(socket) {
   clients[socket.id] = [];
   clients[socket.id]['socket'] = socket;

   socket.on('disconnect', function() {
      console.log(getDateTime()+" INFO  socket.on('disconnect') : client "+socket.id+" disconnected");
      delete clients[socket.id];
      delete phpsessids[socket.phpsessid];
  });
});


function sendMessage(key, message) {
   // emission du message 'key' à tous les clients "authentifies"
   for (var i in clients) {
      try {
         clients[i].socket.emit(key, message.toString());
      }
      catch(err)
      {
         console.log(err.stack);
      }
   }
}
