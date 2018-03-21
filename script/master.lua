local event = require "event"
local channel = require "channel"
local handler = import "handler.master_handler"
local helper = require "helper"
local redis = require "redis"
local redis_watcher = require "redis_watcher"
local util = require "util"
local logger = require "logger"

local master_channel = channel:inherit()

function master_channel:disconnect()
    handler.leave(self)
end

local function accept(_,channel)
    handler.enter(channel)
end


local ok = event.listen(string.format("tcp://0.0.0.0:%d",env.master),4,accept,master_channel)
if not ok then
    event.error("listen error")
end

event.error(string.format("master listen on:%s",env.master))

for i = 1,env.httpd_max do
	event.fork(function ()
		while true do
			event.run_process("./event httpd")
			event.error("httpd down,try again")
			event.sleep(1)
		end
	end)
end
