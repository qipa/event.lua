local util = require "util"

local tinsert = table.insert

local function test(a)
	return function ()
		return a
	end
end

print(util.type(nil))
print(util.type(true))
print(util.type(1))
print(util.type({}))
print(util.type(""))
print(util.type(test))
print(util.type(table.encode({})))
util.time_diff("time",function ()
	for i = 1,1024 * 1024 do
		local a = type({}) == 1
	end
end)
