local util = require "util"
local helper = require "helper"

local event = require "event"



event.fork(function ()
		local now = util.time()
		local count = 0
		for i = 1,1000 do
			event.fork(function ()
				local channel,err  = event.connect("tcp://127.0.0.1:1989",4,true)
			
				-- local str = ""
				-- for i = 1,1024 do
				-- 	str=str.."fuck"
				-- end
				-- channel:write(str)
				-- channel:close()
				-- count = count + 1
				-- if count == 10000 then
				-- 	while true do
				-- 		event.co_clean()
				-- 		collectgarbage("collect")
				-- 		local lua_mem = collectgarbage("count")
				-- 		print(string.format("lua mem:%fkb,c mem:%fkb",lua_mem,helper.allocated()/1024))
				-- 		event.sleep(1)
				-- 	end
				-- end

				if i == 1000 then
					print("done",util.time() - now)
				end
			end)
		end

end)