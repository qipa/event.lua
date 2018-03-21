local event = require "event"
local channel = require "channel"
local handler = import "handler.master_handler"
local helper = require "helper"
local redis = require "redis"
local redis_watcher = require "redis_watcher"
local util = require "util"
local logger = require "logger"



event.fork(function ()
	local channel,err
    while not channel do
        channel,err = event.connect("tcp://127.0.0.1:6379",redis)
        if not channel then
            event.error(string.format("httpd connect master:%d failed:%s",env.master,err))
            event.sleep(1)
        end
    end
--    table.print(channel:cmd("auth","redis123456"))
	event.fork(function ()
	    for i = 1,1024*1024 do
	    	channel:cmd("HMSET","hm","mrq",1,"hx",2)
	    	channel:cmd("HMGET","hm","mrq","hx")
	    end
	end)
    
    event.fork(function ()
    	for i = 1,1024 * 1024 do
    		channel:cmd("PUBLISH","ch",i)
    		event.sleep(0.1)
    	end
    end)
    	
    event.fork(function ()
    	local channel,err
	    while not channel do
	        channel,err = event.connect("tcp://127.0.0.1:6379",redis_watcher)
	        if not channel then
	            event.error(string.format("httpd connect master:%d failed:%s",env.master,err))
	            event.sleep(1)
	        end
	    end
--	    table.print(channel:auth("redis123456"))
	    table.print(channel:subscribe("ch"))
    	while true do
	    	table.print(channel:wait())
	    end
    end)
end)
