local event = require "event"
local util = require "util"

import "handler.logger_handler"

event.fork(function ()
	event.listen(env.logger,4,function (channel)

	end)
end)