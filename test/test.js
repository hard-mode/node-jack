var jack = require('../build/Release/jack.node');
process.stdin.resume();
console.log(jack);
//console.log(jack.Client('foo', {}));
console.log(new jack.Client('test', {}));
