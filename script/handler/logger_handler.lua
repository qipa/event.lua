local event = require "event"


local _M = {}

local _log_ctx = {}

function log(log_type,log)
	local FILE = _log_ctx[log_type]
	if not FILE then
		FILE = assert(io.open(string.format("./log/%s.log",log_type),"a+"))
		_log_ctx[log_type] = FILE
	end
	FILE:write(log.."\r\n")
	FILE:flush()
	print(log)
end

function log_worker(args)
	event.error(table.unpack(args))
end

