util = require 'util'
{FLACStream} = require('bindings')('addon.node')
util.inherits(FLACStream, require('stream').Transform)

module.exports = FLACStream
