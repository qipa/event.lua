local event = require "event"



event.fork(function ()
	local channel = event.connect("tcp://127.0.0.1:8888",4)
	for i = 1,1024 do
		channel:send("handler.data_handler","update_data",{name = "user",id = i,data = {name = "mrq@"..i,id = i}})
	end
	for i = 1,1024 do
		local data =channel:call("handler.data_handler","load_data",{name = "user",id = i})
	end
	print("done")
end)