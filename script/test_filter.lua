local core = require "filter.core"
local dump = require "dump.core"
local util = require "util"
local event = require "event"
local FILE = assert(io.open("filter.lua","r"))
local content = FILE:read("*a")
FILE:close()

local filter_list = dump.unpack(content)

filter = core.create()
local count = 0
for _,word in pairs(filter_list.ForBiddenCharInName) do
	filter:add(word)
	count = count +1 
end
print(count)

filter:dump()
local now = util.time()
for i = 1,1 do
	print(filter:filter("mrq fuck fu k u 法轮功 西藏獨立  孩子  功哈哈 mrq 风骚欲女 阿扁 1"))
end
print("time diff:",(util.time() - now) * 10)
-- event.timer(0,function ()
-- 	event.breakout()
-- end)
-- print("\n")
-- print("------------------------")
-- filter:dump()