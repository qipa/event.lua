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

print(os.tmpname())
-- print(os.execute("ps -elf|grep event"))

local FILE = io.tmpfile()
FILE:write("!@")
FILE:close()

print(util.week_start(os.time()))

util.time_diff("day start",function ()
	for i = 1,1024  * 1024 do
		util.to_date(os.time())
	end
end)

print(os.time())
print(util.daytime(os.time()),util.daytime(1524671999))
-- util.time_diff("day start 1",function ()
-- 	for i = 1,1024  * 1024 do
-- 		util.same_day1(os.time(),1524412800)
-- 	end
-- end)

util.time_diff("decimal_bit",function ()
	for i = 1,1024*1024 do
		util.decimal_bit(1989,2)
	end
end)
