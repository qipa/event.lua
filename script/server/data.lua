local event = require "event"
local util = require "util"

import "handler.data_handler"

event.fork(function ()
	event.listen("ipc://data.ipc",4,function (channel)

	end)
end)