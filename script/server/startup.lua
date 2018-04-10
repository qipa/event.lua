local logger = require "logger"
local event = require "event"

function run()
	local runtime_logger = logger:create("runtime",{level = env.log_lv,addr = env.log_addr},5)
	event.error = function (...)
		runtime_logger:ERROR(...)
	end
end