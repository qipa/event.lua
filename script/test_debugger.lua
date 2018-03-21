local test_handler = import "handler.test_handler"
local event = require "event"
local debugger = require "debugger"














debugger()
event.timer(1,function ()
	test_handler.test(1,2)
end)