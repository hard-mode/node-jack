var jack     = require(__dirname + '/build/Release/jack.node')
  , events   = require('events')
  //, inherits = require('util').inherits;

inherits(jack.Client, events.EventEmitter);
console.log("JACK", jack);
module.exports = jack;

function inherits(target, source) {
  for (var k in source.prototype)
    target.prototype[k] = source.prototype[k];
}
