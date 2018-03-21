local util = require "util"

local LOG_LV_ERROR = 0
local LOG_LV_WARN = 1
local LOG_LV_INFO = 2
local LOG_LV_DEBUG = 3

local LOG_TAG = {
	[LOG_LV_ERROR] 	= "E",
	[LOG_LV_WARN] 	= "W",
	[LOG_LV_INFO] 	= "I",
	[LOG_LV_DEBUG] 	= "D",
}

local _M = {}

_M.LOG_LV_ERROR = LOG_LV_ERROR
_M.LOG_LV_WARN = LOG_LV_WARN
_M.LOG_LV_INFO = LOG_LV_INFO
_M.LOG_LV_DEBUG = LOG_LV_DEBUG

function _M:create(log_type,log_level,log_file,depth)
	local ctx = setmetatable({},{__index = self})
	if log_file then
		if type(log_file) == "string" then
			ctx.FILE = assert(io.open(log_file,"a+"))
			ctx.log_file = log_file
		else
			ctx.log_channel = log_file
		end
	end
	ctx.log_level = log_level or LOG_LV_DEBUG
	ctx.log_type = log_type or "unknown"
	ctx.depth = depth or 4
	return ctx
end

function _M:close()
	if self.FILE then
		self.FILE:flush()
		self.FILE:close()
	end
end

local function get_debug_info(logger)
	local info = debug.getinfo(logger.depth,"lS")
	return info.source,info.currentline
end

local function append_log(logger,log_lv,...)
	local log = table.concat({...},"\t")
	local source,line = get_debug_info(logger)
	local now = math.modf(os.time() / 100)

	local content = string.format("[%s:%s][%s %s:%s] %s",LOG_TAG[log_lv],logger.log_type,os.date("%Y-%m-%d %H:%M:%S",now),source,tostring(line),log)

	if logger.FILE then
		logger.FILE:write(content.."\n")
		logger.FILE:flush()
	else
		if logger.log_channel then
			logger.log_channel:send("handler.logger_handler","log",logger.log_type,content)
		else
			util.print(log_lv,content)
		end
	end
end

function _M:DEBUG(...)
	if self.log_level < LOG_LV_DEBUG then
		return
	end
	append_log(self,LOG_LV_DEBUG,...)
end

function _M:INFO(...)
	if self.log_level < LOG_LV_INFO then
		return
	end
	append_log(self,LOG_LV_INFO,...)
end

function _M:WARN(...)
	if self.log_level < LOG_LV_WARN then
		return
	end
	append_log(self,LOG_LV_WARN,...)
end

function _M:ERROR(...)
	if self.log_level < LOG_LV_ERROR then
		return
	end
	append_log(self,LOG_LV_ERROR,...)
end


return _M