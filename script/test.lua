local util = require "util"

local tinsert = table.insert

local function test(a)
	return function ()
		return a
	end
end

-- util.time_diff("time",function ()
-- 	for i = 1,1024 * 1024 do
-- 		test()
-- 	end
-- end)
