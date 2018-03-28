local event = require "event"
local worker = require "worker"


local mailbox = event.mailbox(function (mailbox,source,session,data,size)
	table.print(table.decode(data,size))
end)

event.fork(function ()

	print(worker.main_create(mailbox:fd(),"test.lua"))
	-- print(worker.create(mailbox:fd(),"test.lua"))
	event.sleep(1)

	local count = 1
	for i = 1,count do
		worker.main_send(0,"handler.test_handler","test_thread_rpc",{fuck = "you"})
		-- worker.main_send(1,"handler.test_handler","test_thread_rpc",{fuck = "me"})
	end
end)