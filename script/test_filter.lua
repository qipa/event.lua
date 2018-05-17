local core = require "filter.core"
local filterex_core = require "filterex.core"
local dump = require "dump.core"
local util = require "util"
local event = require "event"
local helper = require "helper"

local FILE = assert(io.open("./config/filter.lua","r"))
local content = FILE:read("*a")
FILE:close()

local filter_list = dump.unpack(content)

-- local filter = core.create()
-- for _,word in pairs(filter_list.ForBiddenCharInName) do
-- 	filter:add(word)
-- end
-- local lua_mem = collectgarbage("count")
-- event.error(string.format("lua mem:%fkb,c mem:%fkb",lua_mem,helper.allocated()/1024))


-- -- filter:dump()

-- util.time_diff("word filter",function ()
-- 	for i = 1,1 do
-- 		print(filter:filter("mrq fuck fu k u 若Y=0且地图内有地煞星，则下次刷新时不增加该星级的地煞星数量，不回收当前未被挑战西藏獨立成功的地煞星。 法轮功 西藏獨立  孩子 地煞星刷新数量=Y/2（Y为同一星级功的地煞星。 法轮功 西藏獨立  孩子 地煞星刷新数量=Y/2（Y为同一星级的地煞星收到的有效挑战数量的总和，每次刷新操你妈操你妈操你妈操你 妈地煞星后，重置Y=0）  功哈哈 mrq 风骚欲女 阿扁 1"))
-- 	end
-- end)

local filter = filterex_core.create()
-- for _,word in pairs(filter_list.ForBiddenCharInName) do
	-- filter:add(word)
-- end

filter:add("习近平")
print(filter:filter("我爱习近平"))