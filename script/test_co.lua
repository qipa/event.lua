local co_core = require "co.core"
local event = require "event"

function test()
	event.sleep(1)
	event.fork(function ()
		for i = 1,1024 * 1024 do
			local t = {}
		end
		print("!!")
		event.sleep(1)
	end)
end

event.fork(function ()


	co_core.start()
	test()
	local diff = co_core.stop()
	print("diff",diff)
end)