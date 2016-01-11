FLACStream = require "#{__dirname}/../"

stream = new FLACStream
console.log (stream instanceof require('stream').Transform)
stream.on 'bam', (arg) -> console.log "---#{arg}---"
console.log stream._flush()
