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


local agent_channel = channel:inherit()
function agent_channel:disconnect()
	if self.id ~= nil then
		server_handler:agent_server_down(self.id)
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
