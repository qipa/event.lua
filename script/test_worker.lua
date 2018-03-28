local event = require "event"
local worker = require "worker"


local mailbox = event.mailbox(function (...)


end)

event.fork(function ()

	print(worker.create(mailbox:fd(),"test.lua"))
	print(worker.create(mailbox:fd(),"test.lua"))
end)