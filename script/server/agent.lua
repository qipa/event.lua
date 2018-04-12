local event = require "event"
local util = require "util"
local model = require "model"
local protocol = require "protocol"
local mongo = require "mongo"
local route = require "route"
local channel = require "channel"
local startup = import "server.startup"

model.register_binder("scene_channel","id")
model.register_binder("client","id")
model.register_value("client_manager")
model.register_value("db_channel")


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

	startup.connect_server("login")

	startup.connect_server("world")

	startup.connect_server("master")

	local client_manager = event.client_manager(1024)
	client_manager:set_callback(client_accept,client_close,client_data)
	local port,reason = client_manager:start("0.0.0.0",0)
	if not port then
		event.breakout(reason)
	end
	model.set_client_manager(client_manager)

	local login_channel = model.get_login_channel()
	login_channel:send("handler.server_handler","register_agent_server",{ip = "0.0.0.0",port = port})

	local master_channel = model.get_master_channel()
	master_channel:send("handler.server_handler","register_agent_server",{ip = "0.0.0.0",port = port})

	local world_channel = model.get_world_channel()
	world_channel:send("handler.server_handler","register_agent_server",{ip = "0.0.0.0",port = port})
end)
