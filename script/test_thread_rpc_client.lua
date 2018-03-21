local event = require "event"
local worker = require "worker"
local util = require "util"

event.fork(function ()
	local session = 1
	local now = util.time()
	
	local count = 1024
	event.fork(function ()
		for i = 1,count do
			-- table.print(worker.call(1,"handler.test_handler","execute","insert into test (name) values (\"mrq\")"))
			worker.send_back(1,"handler.test_handler","execute",{string.format("select id from test where id == %d",i)},function (result)
				if i == count then
					print("done",util.time() - now)
				end
			end)

		end
	end)
	event.fork(function ()
		for i = 1,count do
			-- table.print(worker.call(2,"handler.test_handler","execute","insert into test (name) values (\"mrq\")"))
			worker.send_back(2,"handler.test_handler","execute",{string.format("select id from test where id == %d",i)},function (result)
				if i == count then
					print("done",util.time() - now)
				end
			end)
		end
	end)

end)