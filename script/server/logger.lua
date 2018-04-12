local event = require "event"
local util = require "util"


event.fork(function ()
	event.listen(env.logger,4,function (channel)

	end)
end)