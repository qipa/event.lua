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

function _M.dump()
	local lua_mem = collectgarbage("count")
	print(str_format("lua mem:%fkb,c mem:%fkb",lua_mem,helper.allocated()/1024))

	print(string.format("monitor:%s",env.name))
	print("time diff:")
	for name,info in pairs(diff_monitor) do
		local average = math.modf(info.total / info.count)
		local min = math.modf(info.min)
		local max = math.modf(info.max)
		print(str_format("%- 50s average:%- 10d,min:%- 10d,max:%- 10d,count:%- 10d",name,average,min,max,info.count))
	end

	print("input flow:")
	for name,info in pairs(input_monitor) do
		local average = math.modf(info.total / info.count)
		print(str_format("%- 50s average:%- 10d,min:%- 10d,max:%- 10d,count:%- 10d",name,average,info.min,info.max,info.count))
	end

	print("output flow:")
	for name,info in pairs(output_monitor) do
		local average = math.modf(info.total / info.count)
		print(str_format("%- 50s average:%- 10d,min:%- 10d,max:%- 10d,count:%- 10d",name,average,info.min,info.max,info.count))
	end

	print("===========================================")
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
	start = true
end

return _M
