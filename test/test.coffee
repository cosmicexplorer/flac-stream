spawn = require('child_process').spawn
Speaker = require 'speaker'

spawn("#{__dirname}/test").stdout.pipe new Speaker
