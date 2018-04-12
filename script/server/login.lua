local event = require "event"
local util = require "util"
local model = require "model"
local protocol = require "protocol"
local mongo = require "mongo"
local channel = require "channel"

local login_handler = import "handler.login_handler"
local server_handler = import "handler.server_handler"

model.register_value("mongodb")
model.register_value("client_manager")
model.register_binder("channel","name")
model.register_binder("login_info","cid")

local rpc_channel = channel:inherit()

function rpc_channel:disconnect()
	if self.name ~= nil then
		if self.name == "agent" then
			server_handler:agent_down(self,self.id)
		end
	end
end

local function channel_accept(_,channel)
	print("channel_accept",channel)
end

local function client_data(cid,message_id,data,size)
	route.dispatch_client(cid,message_id,data,size)
end

local function client_accept(cid,addr)
	login_handler.enter(cid,addr)
end

local function client_close(cid)
	login_handler.leave(cid)
end

event.fork(function ()
	protocol.parse("login")
	protocol.load()
	local mongodb,reason = event.connect(env.mongodb,4,mongo)
	if not mongodb then
		event.breakout(reason)
		return
	end
	mongodb:init("sunset")
	model.set_mongodb(mongodb)

	local ok,reason = event.listen(env.login,4,channel_accept)
	if not ok then
		event.breakout(reason)
		return
	end

	local client_manager = event.client_manager(5000)
	client_manager:set_callback(client_accept,client_close,client_data)
	local ok,reason = client_manager:start("0.0.0.0",1989)
	if not ok then
		event.breakout(reason)
		return
	end
	model.set_client_manager(client_manager)
end)