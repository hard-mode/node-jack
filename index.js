var jack = require(__dirname + '/build/Debug/jack.node');
  //, inherits = require('util').inherits;

inherits(jack.Client, require('eventemitter2').EventEmitter2);
module.exports = jack;

function inherits(target, source) {
  for (var k in source.prototype)
    target.prototype[k] = source.prototype[k];
}
