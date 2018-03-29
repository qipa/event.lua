local event = require "event"




event.listen("tcp://0.0.0.0:8082",4,function (listener,channel)
	print("channel",channel.ip,channel.port)
end,server_channel)