local event = require "event"
local util = require "util"
local model = require "model"
local protocol = require "protocol"
local http = require "http"
local route = require "route"
local channel = require "channel"
local startup = import "server.startup"
local id_builder = import "module.id_builder"
local agent_server = import "module.agent_server"

local mongo_indexes = import "common.mongo_indexes"

model.register_binder("scene_channel","id")
model.register_value("client_manager")


local function client_data(cid,message_id,data,size)
	local ok,err = xpcall(agent_server.dispatch_client,debug.traceback,agent_server,cid,message_id,data,size)
	if not ok then
		event.error(err)
	end
end

local function client_accept(cid,addr)
	local ok,err = xpcall(agent_server.enter,debug.traceback,agent_server,cid,addr)
	if not ok then
		event.error(err)
	end
end

local function client_close(cid)
	local ok,err = xpcall(agent_server.leave,debug.traceback,agent_server,cid)
	if not ok then
		event.error(err)
	end
end


event.fork(function ()
	startup.run(env.mongodb)

	startup.connect_server("login")
	startup.connect_server("world")

	env.dist_id = startup.apply_id()
	id_builder:init(env.dist_id)
	
	local scene_set = {}
	while #scene_set == 0 do
		scene_set = startup.how_many_scene()
		if #scene_set == 0 then
			event.sleep(1)
		else
			break
		end
	end

	event.error(string.format("scene ready:%s",table.concat(scene_set,",")))

	local db_channel = model.get_db_channel()
	db_channel:set_db("agent_user")
	for name,indexes in pairs(mongo_indexes.agent_user) do
		db_channel:ensureIndex(name,indexes)
	end

	local client_manager = event.client_manager(1024)
	client_manager:set_callback(client_accept,client_close,client_data)
	local port,reason = client_manager:start("0.0.0.0",0)
	if not port then
		event.breakout(string.format("%s %s",env.name,reason))
		os.exit(1)
	end
	model.set_client_manager(client_manager)

	local login_channel = model.get_login_channel()
	login_channel:send("module.server_manager","register_agent_server",{ip = "0.0.0.0",port = port,id = env.dist_id})

	local world_channel = model.get_world_channel()
	world_channel:send("module.server_manager","register_agent_server",{ip = "0.0.0.0",port = port,id = env.dist_id})

	agent_server:start(client_manager)
	event.error("start success")
end)
