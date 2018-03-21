local event = require "event"
local importer = require "import"
local test = import "handler.test_reload"


test:test(1)

event.timer(1,function ()
	importer.reload("handler.test_reload")
	test:test(1)
end)