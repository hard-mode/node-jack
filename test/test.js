var jack = require('../index.js');
console.log("\n1. JACK", jack);

console.log("\n2. Class", jack.Client)

console.log("\n3. Proto", jack.Client.prototype)

var client = new jack.Client('test');
console.log("\n4. Instance", client);
console.log("Audio in",  client.registerAudioInput("audio-in"));
console.log("Audio out", client.registerAudioOutput("audio-out"));
console.log("MIDI in",   client.registerMIDIInput("midi-in"));
var midi_out = client.registerMIDIOutput("midi-out");
console.log("MIDI out",  midi_out);

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

var spawn = require('child_process').spawn;
spawn('jack_simple_client');

console.log("\n6. MIDI");

function send_midi () {
  client.sendMidi(midi_out, 144, 127, 127);
}
function connect_midi (port_name) {
  if (port_name === 'midi-monitor:input') {
    console.log("Connecting test:midi-out to midi-monitor:input");
    client.connect('test:midi-out', 'midi-monitor:input');
    client.off('port-registered', connect_midi);
    setInterval(send_midi, 100);
  }
}
client.on('port-registered', connect_midi);
spawn('jack_midi_dump', [], { stdio: 'inherit' });

process.on('SIGINT', function() {
  console.log( "\nGracefully shutting down from SIGINT (Ctrl-C)" );
  client.close();
  process.exit();
})
