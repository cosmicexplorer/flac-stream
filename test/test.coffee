FLACStream = require "#{__dirname}/../"

stream = new FLACStream
console.log (stream instanceof require('stream').Transform)
