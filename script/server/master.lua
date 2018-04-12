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


local scene_channel = channel:inherit()
function scene_channel:disconnect()
	if self.id ~= nil then
		server_handler:scene_server_down(self.id)
	end
end

local function channel_accept()

end

event.fork(function ()
	startup.run()
	protocol.parse("login")
	protocol.load()

	local ok,reason = event.listen(env.master,4,channel_accept,scene_channel)
	if not ok then
		event.breakout(reason)
		return
	end
end)
