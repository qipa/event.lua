local event = require "event"
local util = require "util"
local model = require "model"
local protocol = require "protocol"
local mongo = require "mongo"
local route = require "route"
local startup = import "server.startup"
local protocol_forward = import "server.protocol_forward"

local rpc = import "server.rpc"

model.register_value("mongodb")
model.register_binder("client","id")


local function client_data(client_id,message_id,data,size)
	print(client_id,message_id,data,size)
	local client_info = model.fetch_client_with_id(client_id)
	if not client_info.login then
		rpc.send_login("handler.login_handler","client_forward",{client_id = client_id,message_id = message_id,data = string.copy(data,size)},true)
	else
		local agent_user = model.fetch_agent_user_with_client_id(client_id)
		route.dispatch_client(agent_user,message_id,data,size)
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
	protocol.dumpfile()
	import "handler.agent_handler"
	-- local mongodb,reason = event.connect("tcp://127.0.0.1:10105",4,mongo)
	-- if not mongodb then
	-- 	print(reason)
	-- 	os.exit()
	-- end
	-- mongodb:init("sunset")
	-- model.set_mongodb(mongodb)

	client_manager = event.client_manager(1024)
	client_manager:set_callback(client_accept,client_close,client_data)
	client_manager:start("0.0.0.0",1989)
end)
