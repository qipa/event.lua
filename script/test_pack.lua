local util = require "util"
local event = require "event"
local serialize = require "serialize.core"


local tbl = {
	fuck = {
		career = "code",
		age = 123,
		h = 1.2343,
		w = 333.1
	},
	name = {
		career = "code",
		age = 123,
		h = 1.2343,
		w = 333.1
	}
}

local count = 1024 * 1024
local str
util.time_diff("serialize.pack",function ()
	for i = 1,count do
		str = serialize.tostring(tbl)
	end
end)

util.time_diff("serialize.unpack",function ()
	for i = 1,count do
		serialize.unpack(str)
	end
end)

util.time_diff("table.tostring",function ()
	for i = 1,count do
		str = table.tostring(tbl)
	end
end)

util.time_diff("table.tostring",function ()
	for i = 1,count do
		table.decode(str)
	end
end)

