local event = require "event"
local util = require "util"
local model = require "model"
local protocol = require "protocol"
local mongo = require "mongo"
local route = require "route"
local channel = require "channel"
local startup = import "server.startup"
local server_manager = import "module.server_manager"

local common_channel = channel:inherit()
function common_channel:disconnect()
	if self.id ~= nil then
		if self.name == "agent" then
			server_manager:agent_server_down(self.id)
		elseif self.name == "scene" then
			server_manager:scene_server_down(self.id)
		end
	end
end

local function channel_accept()

end

event.fork(function ()
	startup.run(env.mongodb)

	local ok,reason = event.listen(env.world,4,channel_accept,agent_channel)
	if not ok then
		event.breakout(reason)
		return
	end
end)
