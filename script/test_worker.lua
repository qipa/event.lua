local event = require "event"
local worker = require "worker"
local util = require "util"

event.fork(function ()

	print(worker.create("test.lua"))
	print(worker.create("test.lua"))
	-- print(worker.create(mailbox:fd(),"test.lua"))
	event.sleep(1)

	local now = util.time()
	local count = 1024 * 1024
	for i = 1,1 do
		worker.main_send(0,"handler.test_handler","test_thread_rpc0",{fuck = "you"},function (args)
			if i == count then
				print("done",util.time() - now,i)
				event.breakout()
			end
			
		end)
		-- worker.main_send(1,"handler.test_handler","test_thread_rpc",{fuck = "me"})
	end
end)