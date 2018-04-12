local event = require "event"
local util = require "util"
local model = require "model"
local protocol = require "protocol"
local mongo = require "mongo"
local route = require "route"
local channel = require "channel"
local startup = import "server.startup"
local server_handler = import "handler.server_handler"

model.register_value("db_channel")
model.register_value("logger_channel")
model.register_binder("scene_channel","id")

local rpc_channel = channel:inherit()
function rpc_channel:disconnect()
	model[string.format("set_%s_channel",self.name)](nil)
end

local agent_channel = channel:inherit()
function agent_channel:disconnect()
	if self.id ~= nil then
		server_handler:agent_server_down(self.id)
	end
end

local function channel_accept()

end

event.fork(function ()
	startup.run()
	protocol.parse("login")
	protocol.load()

	local mongodb,reason = event.connect(env.mongodb,4,true,mongodb_channel)
	if not mongodb then
		print(string.format("connect db:%s faield:%s",env.mongodb,reason))
		os.exit()
	end
	mongodb:init("sunset")
	model.set_db_channel(mongodb)

	local ok,reason = event.listen(env.world,4,channel_accept,agent_channel)
	if not ok then
		event.breakout(reason)
		return
	end
end)
