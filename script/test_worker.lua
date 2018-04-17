local event = require "event"
local worker = require "worker"
local util = require "util"

event.fork(function ()
	print(worker.create("worker_main"))
	print(worker.create("worker_main"))

	util.time_diff("diff",function ()
		local count = 1024 * 1024
		for i = 1,1 do
			local result = worker.main_call(0,"handler.test_handler","test_thread_rpc0",{fuck = "you"})
		end
	end)
end)