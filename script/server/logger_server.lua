local event = require "event"
local util = require "util"


event.fork(function ()
	event.listen(env.log_addr,4,function (channel)

	end)
end)