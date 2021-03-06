var Xpl = require("xpl-api");
var commander = require('commander');


commander.option("--networkInterface <name>", "Specify the network interface name");
commander.option("--hubPingDelaySecond <sec>", "");
commander.option("--xplSource <name>", "Specify the source in XPL message");
commander.option("--xplTarget <name>", "Specify the target in XPL message");
commander.option("--device <name>", "");
commander.parse(process.argv);


var xpl = new Xpl(commander);

/*
xpl.on("message", (message) => {
  console.log("Receive message ", message);
});
*/

xpl.on("close", () => {
//  console.log("Receive close even");
  process.exit(0);
});

xpl.bind( xplIsInit );

function xplIsInit() {
  sendMsg();
}

function sendMsg() {
//  xpl.sendXplTrig({ device: "cul", type: "temp", current: 20.4 }, cleanExit);
//  xpl.sendXplStat({ device: "t2", type: "temp", current: 20.4 }, cleanExit);
  if(commander.device==undefined)
     commander.device="XX00";
  xpl.sendXplCmnd({ device: commander.device, request:"current" }, cleanExit);
}

function cleanExit() {
  xpl.close();
}
