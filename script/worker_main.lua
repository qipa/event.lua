local event = require "event"



event.fork(function ()
	print("worker main start")
end)