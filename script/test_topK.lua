local util = require "util"


local arrary = {}
for i = 1,100 do
	table.insert(arrary,math.random(1,10000))
end

util.time_diff("topK",function ()
	util.topK(arrary,10,function (l,r)
		return l < r
	end)
end)

table.print(arrary)
