local event = require "event"
local helper = require "helper"
local logger = require "logger"

local pairs = pairs
local tinsert = table.insert
local tconcat = table.concat
local str_format = string.format

local logger_obj = nil

local timer = nil
local start = false

local diff_monitor = {}
local input_monitor = {}
local output_monitor = {}

local _M = {}

local function update()
	local logs = {}

	local lua_mem = collectgarbage("count")
	tinsert(logs,str_format("lua mem:%fkb,c mem:%fkb",lua_mem,helper.allocated()/1024))

	for name,info in pairs(diff_monitor) do
		local average = info.total / info.count
		tinsert(logs,str_format("%s time diff:average:%f,min:%f,max:%f,count:%d",name,average,info.min,info.max,info.count))
	end

	for name,info in pairs(input_monitor) do
		local average = info.total / info.count
		tinsert(logs,str_format("%s input flow:average:%f,min:%d,max:%d,count:%d",name,average,info.min,info.max,info.count))
	end

	for name,info in pairs(output_monitor) do
		local average = info.total / info.count
		tinsert(logs,str_format("%s output flow:average:%f,min:%d,max:%d,count:%d",name,average,info.min,info.max,info.count))
	end
	logger_obj:WARN(tconcat(logs,"\n"))
end

function _M.report_diff(file,method,diff)
	if not start then
		return
	end
	local name = str_format("%s:%s",file,method)
	local info = diff_monitor[name]
	if not info then
		info = {count = 0,min = nil,max = nil,total = 0}
		diff_monitor[name] = info
	end
	if not info.min or diff < info.min then
		info.min = diff
	end
	if not info.max or diff > info.max then
		info.max = diff
	end
	info.count = info.count + 1
	info.total = info.total + diff
end

function _M.report_input(file,method,size)
	if not start then
		return
	end
	local name = str_format("%s:%s",file,method)
	local info = input_monitor[name]
	if not info then
		info = {count = 0,min = nil,max = nil,total = 0}
		input_monitor[name] = info
	end
	if not info.min or size < info.min then
		info.min = size
	end
	if not info.max or size > info.max then
		info.max = size
	end
	info.count = info.count + 1
	info.total = info.total + size
end

function _M.report_output(file,method,size)
	if not start then
		return
	end
	local name = str_format("%s:%s",file,method)
	local info = output_monitor[name]
	if not info then
		info = {count = 0,min = nil,max = nil,total = 0}
		output_monitor[name] = info
	end
	if not info.min or size < info.min then
		info.min = size
	end
	if not info.max or size > info.max then
		info.max = size
	end
	info.count = info.count + 1
	info.total = info.total + size
end


function _M.start()
	if start then
		return
	end

	timer = event.timer(10,update)
	start = true
	logger_obj = logger:create("monitor")
end

return _M
