local event = require "event"
local channel = require "channel"

local server_channel = channel:inherit()

function server_channel:disconnect()
	channel.disconnect(self)
	print("server channel down")
end

event.listen("ipc://rpc.ipc",4,function (listener,channel)
	print("channel",channel.ip,channel.port)
end,server_channel)