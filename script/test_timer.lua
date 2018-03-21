












local helper = require "helper"

local event = require "event"



event.fork(function ()

		for i = 1,1024 * 200 do
			event.timer(math.random(1,100) / 100,function (timer)
				-- print("timerout")
				-- timer:cancel()
				-- event.breakout()
			end)
		end
		
	while true do
		event.sleep(2)
		-- collectgarbage("collect")
		local lua_mem = collectgarbage("count")
		print(string.format("lua mem:%fkb,c mem:%fkb",lua_mem,helper.allocated()/1024))
	end


end)
