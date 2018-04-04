local event = require "event"
local util = require "util"
local model = require "model"
local protocol = require "protocol"
local mongo = require "mongo"
local protocol_forward = import "server.protocol_forward"
local rpc = import "server.rpc"

model.register_value("mongodb")

local function channel_accept(_,channel)
	print("channel_accept",channel)
end


event.fork(function ()
	protocol.parse("login")
	protocol.load()
	local mongodb,reason = event.connect("tcp://127.0.0.1:10105",4,mongo)
	if not mongodb then
		print(reason)
		os.exit()
	end
	mongodb:init("sunset")
	model.set_mongodb(mongodb)
	event.listen("ipc://login.ipc",4,channel_accept)
end)