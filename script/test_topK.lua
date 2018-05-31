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


local count = 1024 * 1024

util.time_diff("tostring 1",function ()
	for i = 1,count do
		tostring(1234567.89)
	end
end)

util.time_diff("tostring 2",function ()
	for i = 1,count do
		tostring(1233456789)
	end
end)

util.time_diff("tostring 3",function ()
	for i = 1,count do
		string.format("%s",1234567.89)
	end
end)

util.time_diff("tostring 4",function ()
	for i = 1,count do
		string.format("%s",123456789)
	end
end)

util.time_diff("tonumber 1",function ()
	for i = 1,count do
		tonumber("1234567.89")
	end
end)

util.time_diff("tonumber 2",function ()
	for i = 1,count do
		tonumber("123456789")
	end
end)