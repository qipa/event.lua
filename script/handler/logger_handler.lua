local event = require "event"
require "lfs"

local _M = {}

local _log_ctx = {}

function log(_,args)
	local log_type = args[1]
	local log = args[2]
	local FILE = _log_ctx[log_type]
	if not FILE then
		local file = string.format("./log/%s.log",log_type)
		FILE = assert(io.open(file,"a+"))
		_log_ctx[log_type] = FILE
	end
	FILE:write(log.."\r\n")
	FILE:flush()
	print(log)
end

