local util = require "util"
local filter0 = require "filter0.core"
local aoi_core = require "fastaoi.core"
local filter_inst0 = filter0.create()
local aoi = aoi_core.new(256,256,2,5,200)
-- print(util.size_of("111111"))
-- print(util.size_of("111111xvdfgdsssssssssssssssssssssssssssbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"))


-- print(util.size_of(true))
-- print(util.size_of(1989))
-- print(util.size_of(1989.1006))
-- print(util.size_of(filter_inst0))
-- print(util.size_of(aoi))

-- local tbl = {}
-- for i = 1,1024 * 1024 do
-- 	table.insert(tbl,"a111")
-- end
-- print(collectgarbage("count"))

-- print(util.size_of(tbl))

-- local function test()
-- 	tbl = {}
-- end


-- print(util.size_of(test))

local proto = [[
	function sp()

	end
]]

local func,err = load(proto)

print(util.size_of(func))