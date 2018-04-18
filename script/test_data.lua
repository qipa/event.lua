local event = require "event"


event.fork(function ()
	local channel,reason = event.connect("ipc://data.ipc",4,true)
	for i = 1,1024 * 1024 do
		channel:send("handler.data_handler","update",{name = "user",id = i,data = {id = i,name = "mrq"..i}})
	end
end)