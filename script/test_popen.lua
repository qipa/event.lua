local event = require "event"


event.fork(function ()
	print(event.run_process(string.format("ps c -elf|grep %d",env.uid)))
end)