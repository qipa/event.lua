local logger = require "logger"
local event = require "event"
local model = require "model"
local channel = require "channel"

local rpc_channel = channel:inherit()
function rpc_channel:disconnect()
	model[string.format("set_%s_channel",self.name)](nil)
end

function run()
	local runtime_logger = logger:create("runtime",{level = env.log_lv,addr = env.logger},5)
	event.error = function (...)
		runtime_logger:ERROR(...)
	end
end

function connect_server(name)
	model.register_value(string.format("%s_channel"))
	local channel,reason
	local count = 0
	while not channel do
		channel,reason = event.connect(env[name],4,false,rpc_channel)
		if not channel then
			event.error(string.format("connect server:%s %s failed:%s",name,env[name],reason))
			count = count + 1
			if count >= 10 then
				os.exit(1)
			end
			event.sleep(1)
		end
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
					event.error(string.format("connect server:%s %s failed:%s",name,env[name],reason))
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