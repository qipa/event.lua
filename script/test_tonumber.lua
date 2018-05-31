

local util = require "util"


local count = 1024 * 1024

util.time_diff("tonumber 1",function ()
	for i = 1,count do
		util.tonumber("1234567")
	end
end)

util.time_diff("tonumber 2",function ()
	for i = 1,count do
		tonumber("1234567")
	end
end)

