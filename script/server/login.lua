local event = require "event"
local util = require "util"
local model = require "model"
local protocol = require "protocol"
local mongo = require "mongo"
local channel = require "channel"
local route = require "route"
local startup = import "server.startup"

local server_manager = import "module.server_manager"

model.register_value("client_manager")
model.register_binder("agent_channel","id")


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
	event.fork(function ()
		route.dispatch_client(message_id,data,size,cid)
	end)
end

local function client_accept(cid,addr)
	local login_handler = import "handler.login_handler"
	local ok,err = xpcall(login_handler.enter,debug.traceback,cid,addr)
	if not ok then
		event.error(err)
	end
end

local function client_close(cid)
	local login_handler = import "handler.login_handler"
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

	import "handler.login_handler"
end)