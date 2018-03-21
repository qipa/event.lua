












local helper = require "helper"

local event = require "event"



event.fork(function ()
	
		local count = 0
		for i = 1,10000 do
			event.fork(function ()
				local channel,err
				while not channel do
					channel,err = event.connect(string.format("tcp://127.0.0.1:%d",env.httpd_port),4)
					event.sleep(1)
				end
				local str = ""
				for i = 1,1024 do
					str=str.."fuck"
				end
				channel:write(str)
				channel:close()
				count = count + 1
				if count == 10000 then
					while true do
						event.co_clean()
						collectgarbage("collect")
						local lua_mem = collectgarbage("count")
						print(string.format("lua mem:%fkb,c mem:%fkb",lua_mem,helper.allocated()/1024))
						event.sleep(1)
					end
				end
			end)
		end

end)