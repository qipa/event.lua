local filter0 = require "filter0.core"
local filter1 = require "filter1.core"
local dump = require "dump.core"
local util = require "util"
local event = require "event"
local helper = require "helper"


local count = 1024*100

local FILE = assert(io.open("./config/filter.lua","r"))
local content = FILE:read("*a")
FILE:close()

local filter_list = dump.unpack(content)

local forbidden_word = {}
for _,word in pairs(filter_list.ForBiddenCharInName) do
	forbidden_word[word] = true
end

local filter_inst0 = filter0.create()
for word in pairs(forbidden_word) do
	filter_inst0:add(word)
end

local lua_mem = collectgarbage("count")
event.error(string.format("lua mem:%fkb,c mem:%fkb",lua_mem,helper.allocated()/1024))

util.time_diff("filter0",function ()
	for i = 1,count do
		filter_inst0:filter("mrq fuck fu ck u 若Y=0且地图内有地煞星，则下次刷新时不增加该星级的地煞星数量，不回收当前未被挑战西藏獨立成功的地煞星。 法轮功 西藏獨立  孩子 地煞星刷新数量=Y/2（Y为同一星级功的地煞星。 法轮功 西藏獨立  孩子 地煞星刷新数量=Y/2（Y为同一星级的地煞星收到的有效挑战数量的总和，每次刷新操你妈操你妈操你妈操你 妈地煞星后，重置Y=0）  功哈哈 mrq 风骚 欲女 阿 扁 1")
		-- print(filter_inst0:filter("mrq fuck fu ck u 若Y=0且地图内有地煞星，则下次刷新时不增加该星级的地煞星数量，不回收当前未被挑战西藏獨立成功的地煞星。 法轮功 西藏獨立  孩子 地煞星刷新数量=Y/2（Y为同一星级功的地煞星。 法轮功 西藏獨立  孩子 地煞星刷新数量=Y/2（Y为同一星级的地煞星收到的有效挑战数量的总和，每次刷新操你妈操你妈操你妈操你 妈地煞星后，重置Y=0）  功哈哈 mrq 风骚 欲女 阿 扁 1"))
	end
end)

collectgarbage("collect")

local filter_inst1 = filter1.create()
for word in pairs(forbidden_word) do
	filter_inst1:add(word)
end
local lua_mem = collectgarbage("count")
event.error(string.format("lua mem:%fkb,c mem:%fkb",lua_mem,helper.allocated()/1024))

util.time_diff("filter1",function ()
	for i = 1,count do
		filter_inst1:filter("mrq fuck fu ck u 若Y=0且地图内有地煞星，则下次刷新时不增加该星级的地煞星数量，不回收当前未被挑战西藏獨立成功的地煞星。 法轮功 西藏獨立  孩子 地煞星刷新数量=Y/2（Y为同一星级功的地煞星。 法轮功 西藏獨立  孩子 地煞星刷新数量=Y/2（Y为同一星级的地煞星收到的有效挑战数量的总和，每次刷新操你妈操你妈操你妈操你 妈地煞星后，重置Y=0）  功哈哈 mrq 风骚 欲女 阿 扁 1")
		-- print(filter_inst1:filter("mrq fuck fu ck u 若Y=0且地图内有地煞星，则下次刷新时不增加该星级的地煞星数量，不回收当前未被挑战西藏獨立成功的地煞星。 法轮功 西藏獨立  孩子 地煞星刷新数量=Y/2（Y为同一星级功的地煞星。 法轮功 西藏獨立  孩子 地煞星刷新数量=Y/2（Y为同一星级的地煞星收到的有效挑战数量的总和，每次刷新操你妈操你妈操你妈操你 妈地煞星后，重置Y=0）  功哈哈 mrq 风骚 欲女 阿 扁 1"))
	end
end)

