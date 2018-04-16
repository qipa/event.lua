local event = require "event"
local helper = require "helper"
local logger = require "logger"

local logger = logger:create("monitor")

function __init__(self)
	self.timer = event.timer(10,update)
end

function update()
	
	local lua_mem = collectgarbage("count")
	logger:WARN(string.format("%s:lua mem:%fkb,c mem:%fkb",env.name,lua_mem,helper.allocated()/1024))
end