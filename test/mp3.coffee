fs = require 'fs'
lame = require 'lame'
Speaker = require 'speaker'

stream = fs.createReadStream('/home/cosmicexplorer/Music/Astronaut - Rain (MitiS
Remix).mp3').pipe(new lame.Decoder).pipe process.stdout
