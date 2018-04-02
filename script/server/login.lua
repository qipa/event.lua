local event = require "event"
local util = require "util"
local model = require "model"

local rpc = import "server.rpc"

local function channel_accept(_,channel)
	print("channel_accept",channel)
end


event.fork(function ()
	event.listen("ipc://login.ipc",4,channel_accept)
end)