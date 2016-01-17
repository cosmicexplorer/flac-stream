util = require 'util'
{Transform} = require 'stream'
FLACStream = require('bindings')('addon.node').FLACStreamerCreator(Transform)
util.inherits(FLACStream, Transform)
module.exports = FLACStream
