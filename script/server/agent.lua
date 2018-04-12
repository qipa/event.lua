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


model.register_binder("client","id")
model.register_value("client_manager")
model.register_value("db_channel")
model.register_value("logger_channel")
model.register_value("login_channel")
model.register_value("master_channel")
model.register_value("world_channel")
model.register_binder("scene_channel","id")

local rpc_channel = channel:inherit()
function rpc_channel:disconnect()
	model[string.format("set_%s_channel",self.name)](nil)
end

local mongodb_channel = mongo:inherit()
function mongodb_channel:disconnect()
	model.set_db_channel(nil)
	os.exit(1)
end

local function client_data(cid,message_id,data,size)
	local user = model.fetch_agent_user_with_cid(cid)
	if not user then
		route.dispatch_client(message_id,data,size,cid)
	else
		route.dispatch_client(message_id,data,size,user)
	end
end

local function client_accept(id)
	print("client_accept",id)
	model.bind_client_with_id(id,{login = false})
end

local function client_close(id)
	print("client_close",id)
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

	connect_server("login")

	connect_server("world")

	connect_server("master")

	local client_manager = event.client_manager(1024)
	client_manager:set_callback(client_accept,client_close,client_data)
	local port,reason = client_manager:start("0.0.0.0",0)
	if not port then
		print(string.format("agent listen client failed:%s",reason))
		os.exit()
	end
	model.set_client_manager(client_manager)

	local login_channel = model.get_login_channel()
	login_channel:send("handler.server_handler","register_agent",{ip = "0.0.0.0",port = port})
end)
