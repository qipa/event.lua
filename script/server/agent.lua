local event = require "event"
local util = require "util"
local model = require "model"
local protocol = require "protocol"
local mongo = require "mongo"
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
		local name = protocol_forward.forward[message_id]
		local message = protocol.decode[name](data,size)
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
	protocol.parse("login")
	protocol.load()
	-- local mongodb,reason = event.connect("tcp://127.0.0.1:10105",4,mongo)
	-- if not mongodb then
	-- 	print(reason)
	-- 	os.exit()
	-- end
	-- mongodb:init("sunset")
	-- model.set_mongodb(mongodb)

	client_manager = event.client_manager(1024)
	local callback = {
		accept = client_accept,
		close = client_close,
		data = client_data,
	}
	client_manager:set_callback(callback)
	client_manager:start("0.0.0.0",1989)
end)