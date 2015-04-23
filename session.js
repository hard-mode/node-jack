var caches  = require('cache-manager')    // caching engine
  //, deasync = require('deasync')          // async wrappers
  , fs      = require('fs')               // filesystem ops
  , path    = require('path')             // path operation
  , sandbox = require('sandboxed-module') // module sandbox
  , spawn   = require('child_process').spawn
  , vm      = require('vm')               // eval isolation
  , Watcher = require('./watcher')        // watch our code
  , wisp    = require('wisp/compiler');   // lispy language

require('./winston.js');

var Services = function () {
  this.tasks = {}
}

Services.prototype.spawn = function () {
  var child = spawn.apply(null, arguments);
  this.tasks[child.pid] = child;
}

var Session = function (options) {
  
  var included = [options.sessionPath]
    // to keep track of files used on the server side
    // changes to those would trigger full reloads
    , cache = caches.caching({store: 'memory', max: 100, ttl: 100})
    // will attempt to checksum and cache any unchanged
    // files so they're not recompiled every time
    , watcher = new Watcher(options);
    // reload on changes to source code.

  function start () {
    return sandbox.require(
      options.sessionPath,
      { requires: { 'midi':      require('midi')
                  , 'node-jack': require('node-jack')
                  , 'node-osc':  require('node-osc') 
                  , 'tingodb':   require('tingodb')
                  , 'spawn':     new Services() }
      , globals:  { 'stdin':     process.stdin
                  , 'stdout':    process.stdout }
      , sourceTransformers: { wisp: compileWisp
                            , hash: stripHashBang } }
    );
  };

  var app = start();

  // run watcher, compiling assets
  // any time the session code changes, end this process
  // and allow launcher to restart it with the new code
  watcher.watch(options.sessionPath);
  watcher.on('reload', reload);
  watcher.on('update', function (filePath) {
    console.log('updated', filePath);
    if (included.indexOf(filePath) !== -1) reload();
  });

  function reload () {
    if (app.stop) app.stop();
    app = start();
  };

  function stripHashBang (source) {
    // if the first line of a source file starts with #!,
    // remove that line so that it doesn't break the vm.
    return source.replace(/^#!.*/m, '');
  }

  function compileWisp (source) {
    // make sandboxed `require` calls aware of `.wisp` files
    // but don't add require-time compilation like `wisp.engine.node`
    // leaving it to this very sourceTransformer to handle compilation

    var filename = this.filename;

    // the newline allows stripHashBang to work
    src = 'require.extensions[".wisp"]=true;\n';

    // 'this' is bound to the sandboxed module instance
    if (path.extname(filename) === '.wisp') {

      // compute hash value for file contents
      var hash = require('string-hash')(source)
        , key  = "wisp:" + hash + ":" + source.length;

      // try to get cached compiler output
      var cached = false; //get.call(cache, key);
      if (cached) {
        src += cached.item;
      } else {

        // compile and store in cache
        var compiled = wisp.compile(source);

        if (compiled.error) throw new Error(
          "compile error in " + filename + "\n" +
          "  " + compiled.error + "\n");

        if (!compiled) throw new Error(
          "compiler returned nothing for " + filename);

        src += compiled.code;
        cache.set(key, compiled.code, 1000000, function (err) {
          if (err) console.log(
            "error caching " + filename +
            " under " + key, err);
          console.log("cached " + filename + " under " + key);
        });
      }

      src += "\n;require('source-map-support').install();";

      // watch this file and keep track of its inclusion
      if (included.indexOf(filename) === -1) {
        included.push(filename);
        watcher.watch(filename);
      }

    } else { 

      src += source;

    }

    return src;
  }

}


if (require.main === module) {
  var app = new Session(
    { sessionPath: process.env.SESSION }
  );
}
