local event = require "event"
local util = require "util"


event.fork(function ()
	event.listen("ipc://log.ipc",4,function (channel)

	end)
end)