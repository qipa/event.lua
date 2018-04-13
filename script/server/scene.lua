local event = require "event"
local util = require "util"
local model = require "model"
local protocol = require "protocol"
local mongo = require "mongo"
local route = require "route"
local channel = require "channel"
local http = require "http"
local startup = import "server.startup"
local server_manager = import "module.server_manager"

local agent_channel = channel:inherit()
function agent_channel:disconnect()
	if self.id ~= nil then
		server_manager:agent_server_down(self.id)
	end
end

event.fork(function ()
	startup.run(env.mongodb)

	startup.connect_server("world")
	startup.connect_server("master")

	local listener,reason = event.listen(env.scene,4,channel_accept,agent_channel)
	if not listener then
		event.breakout(reason)
		return
	end

	local result,reason = http.post_master("/apply_id")
	if not result then
		print(reason)
		os.exit(1)
	end
	env.dist_id = result.id

	local ip,port = listener:addr()
	local addr_info = {}
	if port == 0 then
		addr_info.file = ip
	else
		addr_info.ip = ip
		addr_info.port = port
	end

	local master_channel = model.get_master_channel()
	master_channel:send("module.server_manager","register_scene_server",addr_info)

	local world_channel = model.get_world_channel()
	world_channel:send("module.server_manager","register_scene_server",addr_info)
end)
