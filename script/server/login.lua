local event = require "event"
local util = require "util"
local model = require "model"
local protocol = require "protocol"
local protocol_forward = import "server.protocol_forward"
local rpc = import "server.rpc"

local function channel_accept(_,channel)
	print("channel_accept",channel)
end


event.fork(function ()
	protocol.parse("login")
	protocol.load()
	event.listen("ipc://login.ipc",4,channel_accept)
end)