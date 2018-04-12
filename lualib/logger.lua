local event = require "event"
local util = require "util"
local model = require "model"
local channel = require "channel"

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


local logger_container = {}


local log_channel = channel:inherit()

function log_channel:disconnect()
	channel.disconnect(self)
	_channel = nil
end

local _M = {}

function _M:create(log_type,depth)
	log_type = log_type or "unknown"
	depth = depth or 4

	local logger = logger_container[log_type]
	if logger then
		return logger
	end

	local ctx = setmetatable({},{__index = self})
	ctx.log_level = env.log_lv or LOG_LV_DEBUG
	ctx.log_type = log_type
	ctx.depth = depth

	logger_container[log_type] = ctx

	return ctx
end

function _M:close()

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
	local logger_channel = model.get_logger_channel()
	if not logger_channel then
		return
	end
	logger_channel:send("handler.logger_handler","log",{logger.log_type,content})
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