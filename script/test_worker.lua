local event = require "event"
local worker = require "worker"
local util = require "util"

event.fork(function ()

	print(worker.create("test.lua"))
	print(worker.create("test.lua"))
	-- print(worker.create(mailbox:fd(),"test.lua"))

	local now = util.time()
	local count = 1024 * 1024
	for i = 1,1 do
		local result = worker.main_call(0,"handler.test_handler","test_thread_rpc0",{fuck = "you"})
		-- local result = worker.main_call(0,"handler.test_handler","test_thread_rpc1",{fuck = "you"})
		if i == count then
			print("done",util.time() - now,i)
			-- event.breakout()
		end
		-- worker.main_send(1,"handler.test_handler","test_thread_rpc",{fuck = "me"})
	end
end)