FLACStream = require "#{__dirname}/../"

fs = require 'fs'
Speaker = require 'speaker'

filename = './01 - Never Ending Circles.flac'

stream = new FLACStream
stream.on 'error', (err) ->
  console.error err
  process.exit -1

console.log (stream instanceof require('stream').Transform)
fs.createReadStream(filename).pipe(stream).pipe(new Speaker)
