
local event = require "event"
local channel = require "channel"
local handler = import "handler.master_handler"
local helper = require "helper"
local redis = require "redis"
local util = require "util"
local logger = require "logger"
local profiler = require "profiler.core"

profiler.start()

local _M = {}

function _M.test4()
	local function test5()
		local b = 1
	local c = 2
	local d = 3
	local e = 4

	end
	local b = 1
	local c = 2
	test5()
	local d = 3
	local e = 4
end

function test3()
	local b = 1
	local c = 2
	_M.test4()
	local d = 3
	local e = 4
	
end

local function test2()
	local b = 1
	local c = 2
	test3()
	local d = 3
	local e = 4
end


local function test1()
	local a = 1
	local b = 2
	return test2()
end

event.timer(0.01,function ()

	for i = 1,1024 * 10 do
			test1()
		end

end)


event.fork(function ()
	while true do
		for i = 1,1024 * 10 do
			test1()
		end
		event.sleep(0.01)
	end

end)

event.timer(300,function (timer)
	timer:cancel()
	profiler.stop("./lua.prof")
end)