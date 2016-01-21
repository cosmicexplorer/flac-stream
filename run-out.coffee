fs = require 'fs'
Speaker = require 'speaker'

fs.createReadStream('test.out').pipe(new Speaker)
