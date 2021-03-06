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

end

local function agent_accept(_,channel)


end

event.fork(function ()
	-- helper.heap.start("scene")

	startup.run(env.monitor,env.mongodb,env.config,env.protocol)

	startup.connect_server("world")

	env.dist_id = startup.apply_id()
	id_builder:init(env.dist_id)

	local addr
	if env.scene == "ipc" then
		addr = string.format("ipc://scene%02d.ipc",env.dist_id)
	else
		addr = "tcp://127.0.0.1:0"
	end
	local listener,reason = event.listen(addr,4,agent_accept,agent_channel)
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

	local world_channel = model.get_world_channel()
	world_channel:send("module.server_manager","register_scene_server",{id = env.dist_id,addr = addr_info})

	import "handler.scene_handler"

	-- local count = 1
	-- while true do
	-- 	helper.heap.dump("timeout")
	-- 	print(event.run_process(string.format("pprof --text ./event scene.%04d.heap",count)))
	-- 	count = count + 1
	-- 	event.sleep(10)
	-- end
end)
