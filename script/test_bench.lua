
local event = require "event"
local cjson = require "cjson"
local util = require "util"
local bson = require "bson"

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

local tbl = {id = 1,rate = 1.323,fuck = 123123.3211,name = "mrq",info = {{age = 28,girl = "hx"},2}}
local str
util.time_diff("table.tostring",function ()
	for i = 1,count do
		str = table.tostring(tbl)
	end
	print("str.len",str:len())
end)

util.time_diff("table.decode",function ()
	for i = 1,count do
		tbl = table.decode(str)
	end
end)

str = "return"..str
util.time_diff("lua.load",function ()
	for i = 1,count do
		tbl = load(str)()
	end
end)

util.time_diff("cjson.encode",function ()
	for i = 1,count do
		str = cjson.encode(tbl)
	end
end)

util.time_diff("cjson.decode",function ()
	for i = 1,count do
		tbl = cjson.decode(str)
	end
	print("str.len",str:len())
end)

util.time_diff("bson.encode",function ()
	for i = 1,count do
		str = bson.encode(tbl)
	end
end)

util.time_diff("bson.decode",function ()
	for i = 1,count do
		tbl = bson.decode(str)
	end

end)

local str = table.tostring(tbl)

local base64
util.time_diff("base64.encode",function ()
	for i = 1,count do
		base64 = util.base64_encode(str)
	end
end)


util.time_diff("base64.decode",function ()
	for i = 1,count do
		str = util.base64_decode(base64)
	end
end)

local hex
util.time_diff("hex.encode",function ()
	for i = 1,count do
		hex = util.hex_encode(str)
	end
end)

util.time_diff("hex.decode",function ()
	for i = 1,count do
		str = util.hex_decode(hex)
	end
end)

print(str)

