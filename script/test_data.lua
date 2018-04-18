local event = require "event"
local helper = require "helper"

event.fork(function ()
	local channel,reason = event.connect("ipc://data.ipc",4,true)
	for j = 1,1024 do
		for i = 1,1024 do
			channel:send("handler.data_handler","update",{name = "user",id = i,data = {id = i,name = "mrq"..i}})
		end
	end


	while true do
		event.clean()
		collectgarbage("collect")
		helper.free()
		local lua_mem = collectgarbage("count")
		print(string.format("%s:lua mem:%fkb,c mem:%fkb",env.name,lua_mem,helper.allocated()/1024))
		event.sleep(1)
	end
end)