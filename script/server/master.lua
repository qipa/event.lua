local event = require "event"
local util = require "util"
local model = require "model"
local protocol = require "protocol"
local mongo = require "mongo"
local route = require "route"
local channel = require "channel"
local startup = import "server.startup"
local server_handler = import "handler.server_handler"


model.register_binder("scene_channel","id")
model.register_binder("agent_channel","id")


local common_channel = channel:inherit()
function common_channel:disconnect()
	if self.id ~= nil then
		if self.name == "agent" then
			server_handler:agent_server_down(self.id)
		elseif self.name == "scene" then
			server_handler:scene_server_down(self.id)
		end
	end
end

local function channel_accept()

end

event.fork(function ()
	startup.run()

	local ok,reason = event.listen(env.master,4,channel_accept,common_channel)
	if not ok then
		event.breakout(reason)
		return
	end
end)
