local core = require "filter.core"
local dump = require "dump.core"
local util = require "util"
local event = require "event"
local FILE = assert(io.open("filter.lua","r"))
local content = FILE:read("*a")
FILE:close()

local filter_list = dump.unpack(content)

filter = core.create()
for _,word in pairs(filter_list.ForBiddenCharInName) do
	filter:add(word)
end


local now = util.time()
for i = 1,1024 * 1024 do
	filter:filter("mrq fuck u 法轮功哈哈 mrq 风骚欲女 习近平哈哈 习近平老婆,风骚 *李*洪*志*阿扁 1")
 -- 	for _, ts in pairs(filter_list.ForBiddenCharInName) do
 -- 		local s = string.lower(ts)
	-- 	if string.find("mrq fuck u 法轮功 哈哈 mrq 风骚欲女 ,风骚", s, 1) then
	-- 		-- print("ddd")
	-- 	end
	-- end

end
print("time diff:",(util.time() - now) * 10)
-- event.timer(0,function ()
-- 	event.breakout()
-- end)
-- print("\n")
-- print("------------------------")
-- filter:dump()