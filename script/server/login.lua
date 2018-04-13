local event = require "event"
local util = require "util"
local model = require "model"
local protocol = require "protocol"
local mongo = require "mongo"
local channel = require "channel"

local startup = import "server.startup"
local login_handler = import "handler.login_handler"
local server_manager = import "module.server_manager"

model.register_value("client_manager")
model.register_binder("agent_channel","id")
model.register_binder("login_info","cid")

local common_channel = channel:inherit()
function common_channel:disconnect()
	if self.id ~= nil then
		if self.name == "agent" then
			server_manager:agent_server_down(self.id)
		end
	end
end


local function channel_accept(_,channel)
	
end

local function client_data(cid,message_id,data,size)
	local ok,err = xpcall(route.dispatch_client,debug.traceback,cid,message_id,data,size)
	if not ok then
		event.error(err)
	end
end

local function client_accept(cid,addr)
	local ok,err = xpcall(login_handler.enter,debug.traceback,cid,addr)
	if not ok then
		event.error(err)
	end
end

local function client_close(cid)
	local ok,err = xpcall(login_handler.leave,debug.traceback,cid)
	if not ok then
		event.error(err)
	end
end

event.fork(function ()
	startup.run(env.mongodb)

	local ok,reason = event.listen(env.login,4,channel_accept,common_channel)
	if not ok then
		event.breakout(reason)
		return
	end

	local client_manager = event.client_manager(5000)
	client_manager:set_callback(client_accept,client_close,client_data)
	local ok,reason = client_manager:start(table.unpack(env.login_addr))
	if not ok then
		event.breakout(string.format("login listen client failed:%s",reason))
		return
	end
	event.error(string.format("login listen client success",reason))
	model.set_client_manager(client_manager)
end)