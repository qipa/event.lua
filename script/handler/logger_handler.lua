local event = require "event"
local util = require "util"

_log_ctx = _log_ctx or {}
_log_FILE = _log_FILE or nil

function __init__(self)

end

function log(_,args)
	local log_lv = args[1]
	local log_type = args[2]
	if log_type == "monitor" then
		return
	end
	local log = args[3]
	local FILE = _log_ctx[log_type]
	if not FILE then
		if env.log_path then
			local file = string.format("%s/%s.log",env.log_path,log_type)
			FILE = assert(io.open(file,"a+"))
			_log_ctx[log_type] = FILE
		end
	end

	if FILE then
		FILE:write(log.."\r\n")
		FILE:flush()
	else
		util.print(log_lv,log)
	end
end

