local event = require "event"
local util = require "util"
local model = require "model"
local protocol = require "protocol"
local mongo = require "mongo"
local route = require "route"
local channel = require "channel"
local startup = import "server.startup"
local server_handler = import "handler.server_handler"
local rpc = import "server.rpc"


model.register_value("logger_channel")
model.register_binder("scene_channel","id")

local rpc_channel = channel:inherit()
function rpc_channel:disconnect()
	model[string.format("set_%s_channel",self.name)](nil)
end

local scene_channel = channel:inherit()
function scene_channel:disconnect()
	if self.id ~= nil then
		server_handler:scene_server_down(self.id)
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

	local ok,reason = event.listen(env.master,4,channel_accept,scene_channel)
	if not ok then
		print(reason)
		event.breakout()
		return
	end

end)
