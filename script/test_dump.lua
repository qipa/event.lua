local util = require "util"
local dump = require "dump.core"

local count = 1024 * 1024

local tbl = {id = 1,rate = 1.323,fuck = 123123.3211,name = "mrq",info = {{age = 28,girl = "hx"},2}}

local str
util.time_diff("dump.tostring",function ()
	for i = 1,count do
		str = dump.tostring(tbl)
	end
end)

util.time_diff("dump.unpack",function ()
	for i = 1,count do
		tbl = dump.unpack(str)
	end
end)