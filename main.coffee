util = require 'util'
{FLACStream} = require('bindings')('addon.node')
{Transform} = require 'stream'
util.inherits(FLACStream, Transform)
module.exports = FLACStream
