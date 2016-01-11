spawn = require('child_process').spawn
Speaker = require 'speaker'

# spawn("#{__dirname}/test").stdout.pipe new Speaker
proc = spawn("#{__dirname}/test")
proc.stdout.pipe process.stdout
proc.stderr.pipe process.stderr
proc.on 'close', (code) -> process.exit code
