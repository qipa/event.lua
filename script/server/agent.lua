local event = require "event"
local util = require "util"
local model = require "model"
local protocol = require "protocol"
local http = require "http"
local route = require "route"
local channel = require "channel"
local startup = import "server.startup"
local id_builder = import "module.id_builder"


model.register_binder("scene_channel","id")
model.register_binder("client","id")
model.register_value("client_manager")


local function client_data(cid,message_id,data,size)
	local user = model.fetch_agent_user_with_cid(cid)
	if not user then
		route.dispatch_client(message_id,data,size,cid)
	else
		route.dispatch_client(message_id,data,size,user)
	end
end

local function client_accept(id,addr)
	model.bind_client_with_id(id,{login = false,addr = addr})
	local agent_handler = import "handler.agent_handler"
	agent_handler.enter(id,addr)
end

local function client_close(id)
	model.unbind_client_with_id(id)
	local agent_handler = import "handler.agent_handler"
	agent_handler.leave(id)
end


event.fork(function ()
	startup.run(env.mongodb)

	startup.connect_server("login")
	startup.connect_server("world")
	startup.connect_server("master")

	env.dist_id = startup.apply_id()
	id_builder:init(env.dist_id)

	local login_channel = model.get_login_channel()
	login_channel:send("module.server_manager","register_agent_server",{ip = "0.0.0.0",port = port,id = env.dist_id})

	local master_channel = model.get_master_channel()
	master_channel:send("module.server_manager","register_agent_server",{ip = "0.0.0.0",port = port,id = env.dist_id})

	local world_channel = model.get_world_channel()
	world_channel:send("module.server_manager","register_agent_server",{ip = "0.0.0.0",port = port,id = env.dist_id})

	import "handler.agent_handler"
	
	local scene_set
	while true do
		local result,reason  = http.post_master("/howmany_scene")
		if not result then
			event.error(string.format("howmany_scene error:%s",reason))
			event.breakout(reason)
			return
		end
		if #result == 0 then
			event.sleep(1)
		else
			scene_set = result
			break
		end
	end

	event.error(string.format("scene already:%s",table.concat(scene_set,",")))

	local client_manager = event.client_manager(1024)
	client_manager:set_callback(client_accept,client_close,client_data)
	local port,reason = client_manager:start("0.0.0.0",0)
	if not port then
		event.breakout(string.format("%s %s",env.name,reason))
		os.exit(1)
	end
	model.set_client_manager(client_manager)

	event.error("start success")
end)
