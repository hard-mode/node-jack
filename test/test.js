var jack = require('../index.js');
console.log("\n1. JACK", jack);

console.log("\n2. Class", jack.Client)

console.log("\n3. Proto", jack.Client.prototype)

var client = new jack.Client('test');
console.log("\n4. Instance", client);
console.log(client.registerAudioInput("audio-in"));
console.log(client.registerAudioOutput("audio-out"));
console.log(client.registerMIDIInput("midi-in"));
console.log(client.registerMIDIOutput("midi-out"));

client.on('client-registered', function () {
  console.log("Client registered", arguments)
});

client.on('client-unregistered', function () {
  console.log("Client unregistered", arguments)
});

client.on('port-registered', function (port) {
  console.log("Port registered", arguments);
  //client.disconnect(port, "system:playback_1");
  //client.disconnect(port, "system:playback_2");
});

client.on('port-unregistered', function () {
  console.log("Port registered", arguments)
});

client.on('connect', function () {
  console.log("Ports connected", arguments)
});

client.on('disconnect', function () {
  console.log("Ports disconnected", arguments)
});

console.log("\n5. Callbacks");

require('child_process').spawn('jack_simple_client');

process.on('SIGINT', function() {
  console.log( "\nGracefully shutting down from SIGINT (Ctrl-C)" );
  client.close();
  process.exit();
})
