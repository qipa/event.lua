local logger = require "logger"
local event = require "event"
local model = require "model"
local channel = require "channel"
local mongo = require "mongo"
local protocol = require "protocol"
local monitor = require "monitor"
local util = require "util"
local http = require "http"

local rpc_channel = channel:inherit()
function rpc_channel:disconnect()
	model[string.format("set_%s_channel",self.name)](nil)
	event.wakeup(self.monitor)
end

local mongodb_channel = mongo:inherit()
function mongodb_channel:disconnect()
	model.set_db_channel(nil)
	os.exit(1)
end

function run(db_addr)
	connect_server("logger")
	
	local runtime_logger = logger:create("runtime",5)
	event.error = function (...)
		runtime_logger:ERROR(...)
	end

	if db_addr then
		model.register_value("db_channel")
		local db_channel,reason = event.connect(db_addr,4,true,mongodb_channel)
		if not db_channel then
			print(string.format("%s connect db:%s faield:%s",env.name,env.mongodb,reason))
			os.exit()
		end
		db_channel:init("sunset")
		model.set_db_channel(db_channel)
	end

	protocol.parse("login")
	protocol.load()
end

function apply_id()
	local count = 0
	local result,reason
	while not result do
		result,reason = http.post_master("/apply_id")
		if not result then
			event.error(string.format("apply id error:%s",reason))
			count = count + 1
			event.sleep(1)
		end
	end
	if not result then
		os.exit(1)
	end
	return result.id
end

function connect_server(name)
	model.register_value(string.format("%s_channel",name))
	local channel,reason
	local count = 0
	while not channel do
		channel,reason = event.connect(env[name],4,false,rpc_channel)
		if not channel then
			event.error(string.format("%s connect server:%s %s failed:%s",env.name,name,env[name],reason))
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
					event.error(string.format("%s connect server:%s %s failed:%s",env.name,name,env[name],reason))
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