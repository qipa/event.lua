local co_core = require "co.core"
local event = require "event"

function test()
	event.fork(function ()
		event.sleep(1)
	end)
end

event.fork(function ()


	co_core.start()
	test()
	local diff = co_core.stop()
	print("diff",diff)
end)