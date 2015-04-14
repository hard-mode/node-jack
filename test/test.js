var jack = require('../index.js');
console.log("1. JACK", jack);

console.log("2. Class", jack.Client)

console.log("3. Proto", jack.Client.prototype)

var client = new jack.Client('test');
console.log("4. Instance", client);

process.stdin.resume();
function cb () { console.log ("CALLBACK", this, arguments ) }

client.on('client-registered', function () {
  console.log("Client registered", arguments)
});

client.on('client-unregistered', function () {
  console.log("Client unregistered", arguments)
});

client.on('port-registered', function () {
  console.log("Port registered", arguments)
});

client.on('port-unregistered', function () {
  console.log("Port registered", arguments)
});

console.log("5. Callbacks");

require('child_process').spawn('jack_simple_client');
