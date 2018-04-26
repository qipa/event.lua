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

function run(db_addr,config_path,protocol_path)
	connect_server("logger")
	
	local runtime_logger = logger:create("runtime",5)
	event.error = function (...)
		runtime_logger:ERROR(...)
	end

	if config_path then
		_G.config = {}
		local list = util.list_dir(config_path,true,"lua",true)
		for _,path in pairs(list) do
			local file = table.remove(path:split("/"))
			local name = file:match("%S[^%.]+")
			local data = loadfile(path)()
			_G.config[name] = data
		end
	end

	if protocol_path then
		local list = util.list_dir(protocol_path,true,"protocol",true)
		for _,file in pairs(list) do
			protocol.parse(file)
		end
		protocol.load()
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

		event.error(string.format("connect mongodb:%s success",db_addr))
	end
end

function apply_id()
	local world_channel = model.get_world_channel()
	local id = world_channel:call("module.server_manager","apply_id")
	return id
end

function how_many_agent()
	local world_channel = model.get_world_channel()
	return world_channel:call("module.server_manager","how_many_agent")
end

function how_many_scene()
	local world_channel = model.get_world_channel()
	return world_channel:call("module.server_manager","how_many_scene")
end

function connect_server(name)
	model.register_value(string.format("%s_channel",name))
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