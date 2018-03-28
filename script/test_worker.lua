local event = require "event"
local worker = require "worker"


event.fork(function ()

	print(worker.main_create("test.lua"))
	-- print(worker.create(mailbox:fd(),"test.lua"))
	event.sleep(1)

	local count = 1
	for i = 1,count do
		worker.main_call(0,"handler.test_handler","test_thread_rpc",{fuck = "you"},function (args)
			print("rpc return",args)
		end)
		-- worker.main_send(1,"handler.test_handler","test_thread_rpc",{fuck = "me"})
	end
end)