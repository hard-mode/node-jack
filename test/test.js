var jack = require('../index.js');
console.log("1. JACK", jack);

console.log("2. Class", jack.Client)

console.log("3. Proto", jack.Client.prototype)

var client = new jack.Client('test');
console.log("4. Instance", client);

process.stdin.resume();
client.onclient = function () { console.log("CLIENT!") }
client.onport   = function () { console.log("PORT!")   }
client.on('foo', function () { console.log("DA FOO") })
console.log("5. onclient, onport");

require('child_process').spawn('jack_simple_client');
