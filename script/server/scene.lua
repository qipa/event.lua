local event = require "event"
local util = require "util"
local model = require "model"
local protocol = require "protocol"
local mongo = require "mongo"
local route = require "route"
local channel = require "channel"
local http = require "http"
local helper = require "helper"
local startup = import "server.startup"
local server_manager = import "module.server_manager"
local id_builder = import "module.id_builder"
local mongo_indexes = import "common.mongo_indexes"

local agent_channel = channel:inherit()
function agent_channel:disconnect()
	if self.id ~= nil then
		server_manager:agent_server_down(self.id)
	end
end

event.fork(function ()
	-- helper.heap.start("scene")

	startup.run(env.mongodb)

	startup.connect_server("world")
	startup.connect_server("master")

	env.dist_id = startup.apply_id()
	id_builder:init(env.dist_id)

	local addr
	if env.scene == "ipc" then
		addr = string.format("ipc://scene%02d.ipc",env.dist_id)
	else
		addr = "tcp://127.0.0.1:0"
	end
	local listener,reason = event.listen(addr,4,channel_accept,agent_channel)
	if not listener then
		event.breakout(reason)
		return
	end

	local ip,port = listener:addr()
	local addr_info = {}
	if port == 0 then
		addr_info.file = ip
	else
		addr_info.ip = ip
		addr_info.port = port
	end

	local master_channel = model.get_master_channel()
	master_channel:send("module.server_manager","register_scene_server",{id = env.dist_id,addr = addr_info})

	local world_channel = model.get_world_channel()
	world_channel:send("module.server_manager","register_scene_server",{id = env.dist_id,addr = addr_info})

	local db_channel = model.get_db_channel()
	db_channel:set_db("scene_user")
	for name,indexes in pairs(mongo_indexes.scene_user) do
		db_channel:ensureIndex(name,indexes)
	end

	import "handler.scene_handler"

	-- event.timer(10,function ()
	-- 	helper.heap.dump("scene")
	-- end)
end)
