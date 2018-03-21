












local helper = require "helper"

local event = require "event"


event.fork(function (args)
	for i = 1,1024 do
		event.fork(function ()
			while true do
				event.sleep(math.random(1,100) / 100)
			end
		end)
	end

	while true do
		event.sleep(5)
		-- collectgarbage("collect")
		event.breakout()
		local lua_mem = collectgarbage("count")
		print(string.format("lua mem:%fkb,c mem:%fkb",lua_mem,helper.allocated()/1024))
	end

end,"fuck")
