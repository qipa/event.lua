local util = require "util"

local arrary = {}
for i = 1,100000 do
	table.insert(arrary,math.random(1,10000))
end

util.time_diff("topK",function ()
	util.topK(arrary,100,function (l,r)
		return l < r
	end)
end)

util.time_diff("sort",function ()
	table.sort(arrary,function (l,r)
		return l < r
	end)
end)
