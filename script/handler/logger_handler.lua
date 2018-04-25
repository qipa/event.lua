local event = require "event"
local util = require "util"

local strformat = string.format
local tostring = tostring
local os_time = os.time
local os_date = os.date

_log_ctx = _log_ctx or {}
_log_FILE = _log_FILE or nil

function __init__(self)

end

function log(_,args)
	local log_lv = args.log_lv
	local log_type = args.log_type
	local log_tag = args.log_tag
	local source_file = args.source_file
	local source_line = args.source_line
	
	local log = args[3]
	local FILE = _log_ctx[log_type]
	if not FILE then
		if env.log_path then
			local file = string.format("%s/%s.log",env.log_path,log_type)
			FILE = assert(io.open(file,"a+"))
			_log_ctx[log_type] = FILE
		end
	end

	local content
	if source_file then
		content = strformat("[%s:%s][%s %s %s:%d] %s",log_tag,log_type,os_date("%Y-%m-%d %H:%M:%S",args.time),args.source_name,source_file,source_line,args.log)
	else
		content = strformat("[%s:%s][%s %s] %s",log_tag,log_type,os_date("%Y-%m-%d %H:%M:%S",args.time),args.source_name,args.log)
	end

	if FILE then
		FILE:write(content)
		FILE:write("\r\n")
		FILE:flush()
	else
		util.print(log_lv,content)
	end
end

