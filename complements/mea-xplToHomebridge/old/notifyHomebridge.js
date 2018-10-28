var http = require('http');
var commander = require('commander');
var url = require("url");

const contentType={'content-type':'application/json'}
const HTTP_PORT=7102

var server = http.createServer(requestHandler);

server.listen(HTTP_PORT, function() {
      console.log("Server listening on port #"+HTTP_PORT);
   }
);

function requestHandler(request, response)
{
   console.log( "request informations: " +
               " method=" + request.method +
               " url=" + request.url +
               " content-type=" + request.headers['content-type'] +
               " authorization=" + request.headers['authorization']);
  var surl = request.url.toLowerCase().split('/');
  var data=''
  request.on("readable", function() {
     var _data=request.read();
     if(_data)
        data+=_data.toString('utf8');
  });

   request.on('end', function() {
      try {
         var selector=surl[1]
         try {
            data=JSON.parse(data);
         }
         catch(e) {
            data=undefined
         }
         surl.splice(0,2)
         switch(selector) {
            case "callback":
               processCallbackData(data)
               response.writeHead(200, contentType);
               response.end('{"status":"OK"}');
               break;
            default:
               response.writeHead(404, contentType);
               response.end('{"status":"error"}');
               break;
         }
      }
      catch(e) {
         response.writeHead(400, contentType);
         response.end('{"status":"error", "message":'+e.message+'}');
         console.log(e);
      }
  });
}

function onsuccess_notification(data) {
   console.log("success:",data);
} 

function onerror_notification(data) {
   console.log("error:",data);
} 


function processCallbackData(data) {

   clearInterval(timer);

   console.log(JSON.stringify(data, null, 3));
/*
{
   "message": {
      "timestamp": 1540496916394,
      "from": {
         "address": "192.168.0.51",
         "family": "IPv4",
         "port": 60765,
         "size": 113
      },
      "headerName": "xpl-trig",
      "header": {
         "hop": "1",
         "source": "mea-edomus.home",
         "target": "*"
      },
      "bodyName": "sensor.basic",
      "body": {
         "device": "o02a",
         "current": "low",
         "type": "input",
         "last": "high"
      }
   },
   "counter": 5,
   "callbackid": "eb1b04582a222772fa563caa955116c3"
}
*/
   try {
      var notificationID=data.message.body.device;
      var onoff=data.message.body.current;
      var flag=false 
      switch(onoff)
      {
         case "low":
         case "off":
            onoff=false;
            flag=true;
            break;
         case "high":
         case "on":
            onoff=true;
            flag=true;
            break;
         default:
            break;
      }
      if(flag===true) {
         callHttp("http://localhost:7103/"+notificationID, "POST", {characteristic: "On", value: onoff}, onsuccess_notification, onerror_notification );
      } 
   }
   catch(e) { console.log(e); }

   if(data.counter<5) {
      callHttp("http://localhost:7101/xpl/callback/"+data.callbackid, "POST", '{"counter":10}', undefined, undefined );
   }

   timer=setInterval(watchBridge, WATCHBRIDGEINTERAL*1000);
}


function callHttp(serverUrl, method, data, onsuccess, onerror) {
   var options = url.parse(serverUrl);
   options.method=method;

   var req=http.request(options, (response) => {
      var str='';
      response.on('data', function(chunk) {
         str+=chunk;
      });
      response.on('end', function() {
         if(typeof(onsuccess)=="function") {
            var _str="" 
            try { 
               _str=JSON.parse(str)
            }
            catch(e) {
               _str=str
            }
            onsuccess(_str);
         }
      });
      response.on('error', function(e) {
         console.log("error:", e.message);
         if(typeof(onerror)=="function")
            onerror(e);
      });
   });
   req.on('error', function(e) {
      console.log("error:", e.message);
      if(typeof(onerror)=="function")
         onerror(e);
   });
   if(method!='GET' && method!='DELETE')
         req.write(JSON.stringify(data));
   req.end();
}


function watchBridge(){
   if(typeof(callbackContext)=="object" && callbackContext.id!=undefined) {
      callHttp("http://localhost:7101/xpl/callback/"+callbackContext.id, "GET", null, checkConnection, null);
   }
   else {
      callbackContext=undefined;
   }

   if(callbackContext==undefined) {
      setCallback();
   }
}


var callbackdata={
   url:"http://localhost:7102/callback",
   filter: { device:"^o.*",
             schema:"sensor\\..*",
             source:".*"
   },
   timeout:1000,
   counter:5
}


function setCallback_onerror(e) {
   console.log(e.message);
//   process.exit(0);
}


function setCallback_onsuccess(data) {
   if(data.status=="error") {
      if(data.errorcode==1) {
         if(callbackContext!=false) {
            callbackContext=false;
            callHttp("http://localhost:7101/xpl/callback/"+data.callbackId, "DELETE", null, function(_data) {
               callHttp("http://localhost:7101/xpl/callback", "PUT", callbackdata, setCallback_onsuccess, setCallback_onerror);
            }, setCallback_onerror);
         }
      }
   }
   else {
      callbackContext=data;
   }
}


function setCallback() {
   callHttp("http://localhost:7101/xpl/callback", "PUT", callbackdata, setCallback_onsuccess, setCallback_onerror);
}


const WATCHBRIDGEINTERAL=60

function checkConnection(data) {
   if(data.status=="error") {
      setCallback();
   }
}

var timer=setInterval(watchBridge, WATCHBRIDGEINTERAL*1000);

var callbackContext=undefined;
setCallback();
