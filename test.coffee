#!/usr/bin/env iced
# probably optimal is UV_THREADPOOL_SIZE=8
crypto = require("crypto")
mod = require("./index")
console.log "UV_THREADPOOL_SIZE=#{process.env.UV_THREADPOOL_SIZE}"

sha2_base = (buf, cb)->
  cb null, crypto.createHash("sha256").update(buf).digest()

sha2_mod = (buf, cb)->
  mod.sha2 buf, cb

cb = (err)->throw err if err
in_buf_size = 10*1024 # will be rewritten
# duration    = 10000
duration    = 1000 # быстрее результаты, чуть меньше точность
batch_size  = 1000
# ###################################################################################################
#    correctness
# ###################################################################################################
arg = Buffer.from "test"
await sha2_base arg, defer(err, res1); return cb err if err
await sha2_mod  arg, defer(err, res2); return cb err if err

if !res1.equals res2
  perr "incorrect result"
  process.exit(1)

# ###################################################################################################
#    bench
# ###################################################################################################
bench_seq = (name, fn, cb)->
  start_ts = Date.now()
  arg = Buffer.alloc in_buf_size
  
  i = 0
  loop
    arg.writeUInt32BE i, 0
    await fn arg, defer(err); return cb err if err
    
    if i % 100 == 0
      elp_ts = Date.now() - start_ts
      break if elp_ts > duration
    i++
  
  hashrate = i/(elp_ts/100)
  console.log "seq #{name} #{hashrate.toFixed(2)}"
  cb()

bench_par = (name, fn, cb)->
  start_ts = Date.now()
  
  hash_count = 0
  loop
    await
      for i in [0 ... batch_size]
        arg = arg_list[i]
        fn arg, defer()
    
    hash_count += batch_size
    elp_ts = Date.now() - start_ts
    break if elp_ts > duration
  
  hashrate = hash_count/(elp_ts/100)
  console.log "par #{name} #{hashrate.toFixed(2)}"
  cb()

for in_buf_size in [80, 1024, 2*1024, 10*1024, 100*1024]
  console.log ""
  console.log "in_buf_size #{in_buf_size}"
  
  arg_list = []
  res_list = []
  for i in [0 ... batch_size]
    arg_list.push buf = Buffer.alloc in_buf_size
    buf.writeUInt32BE i, 0
    res_list.push buf = Buffer.alloc 32
  
  await bench_seq "base", sha2_base, defer(err); return cb err if err
  # очень медленно, печально смотреть
  # await bench_seq "mod ", sha2_mod , defer(err); return cb err if err
  
  # await bench_par "base", sha2_base, defer(err); return cb err if err
  await bench_par "mod ", sha2_mod , defer(err); return cb err if err
  
  # ###################################################################################################
  #    special no_alloc
  # ###################################################################################################
  start_ts = Date.now()
  
  hash_count = 0
  loop
    await
      for i in [0 ... batch_size]
        arg = arg_list[i]
        res = res_list[i]
        mod.sha2_no_alloc arg, res, defer()
    
    hash_count += batch_size
    elp_ts = Date.now() - start_ts
    break if elp_ts > duration
  
  hashrate = hash_count/(elp_ts/100)
  console.log "no_alloc #{hashrate.toFixed(2)}"
