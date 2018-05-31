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

local tbl = {a = 1,b = "str111111",c = {d = 1,e = 1,f = 1 ,s = 1,str = "str111111"},fuc = util.topK}

tbl.tbl = tbl

print(util.size_of(tbl))

local function test()
	tbl = {}
end


-- print(util.size_of(test))

