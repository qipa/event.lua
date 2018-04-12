local event = require "event"
local util = require "util"
local model = require "model"
local protocol = require "protocol"
local mongo = require "mongo"
local route = require "route"
local channel = require "channel"
local startup = import "server.startup"
local protocol_forward = import "server.protocol_forward"

local rpc = import "server.rpc"


model.register_value("db_channel")
model.register_value("logger_channel")
model.register_value("master_channel")
model.register_value("world_channel")


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

local function connect_server(name)
	local channel,reason = event.connect(env[name],4,true,rpc_channel)
	if not channel then
		print(string.format("connect server:%s %s faield:%s",name,env[name],reason))
		os.exit()
	end
	channel.name = name
	channel.monitor = event.gen_session()
	model[string.format("set_%s_channel",name)](channel)

	event.fork(function ( ... )
		event.wait(channel.monitor)

		while true do
			local channel,reason
			while not channel do
				channel,reason = event.connect(env[name],4,false,rpc_channel)
				if not channel then
					print(string.format("connect server:%s %s faield:%s",name,env[name],reason))
					event.sleep(1)
				end
			end
			channel.name = name
			channel.monitor = event.gen_session()
			model[string.format("set_%s_channel",name)](channel)
			event.wait(channel.monitor)
		end
	end)
end


event.fork(function ()
	startup.run()
	protocol.parse("login")
	protocol.load()
	protocol.dumpfile()

	local mongodb,reason = event.connect(env.mongodb,4,true,mongodb_channel)
	if not mongodb then
		print(string.format("connect db:%s faield:%s",env.mongodb,reason))
		os.exit()
	end
	mongodb:init("sunset")
	model.set_db_channel(mongodb)

	connect_server("world")

	connect_server("master")

	local listener,reason = event.listen(env.scene,4,channel_accept,agent_channel)
	if not listener then
		print(reason)
		event.breakout()
		return
	end

	local ip,port = listener:addr()
	local addr_info = {}
	if port == 0 then
		addr_info.file = ip
	else
		addr_info.ip = ip
		addr_info.port = port
	end

	local master_channel = model.get_master_channel()
	master_channel:send("handler.server_handler","register_scene_server",addr_info)
end)
