local event = require "event"




event.fork(function ()
	event.listen("tcp://127.0.0.1:8888",4,function (channel)
		print("channel come",channel.ip)
	end)
end)