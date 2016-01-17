FLACStream = require "#{__dirname}/../"

fs = require 'fs'
Speaker = require 'speaker'

filename = "#{__dirname}/01 - Never Ending Circles.flac"

# TODO: fix this requirement!
stream = new FLACStream {}
stream.on 'error', (err) ->
  console.error err.stack
  process.exit -1
stream.on 'metadata', console.log

console.log (stream instanceof require('stream').Transform)
fs.createReadStream(filename).pipe(stream).pipe(new Speaker)
