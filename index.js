var mod = require("./build/Release/module");

mod.sha2 = function(buf, cb) {
  var result = Buffer.alloc(32)
  mod.sha2_no_alloc(buf, result, function(err){
    if (err) return cb(err);
    cb(null, result);
  })
}

module.exports = mod;
