var jack = require('../index.js');
console.log("1. JACK", jack);

console.log("2. Class", jack.Client)

console.log("3. Proto", jack.Client.prototype)

var client = new jack.Client('test');
console.log("4. Instance", client);

process.stdin.resume();
function cb () { console.log ("CALLBACK", this, arguments ) }
client.on('client-registered',   cb);
client.on('client-unregistered', cb);
client.on('port-registered',     cb);
client.on('port-unregistered',   cb);
console.log("5. Callbacks");

require('child_process').spawn('jack_simple_client');
