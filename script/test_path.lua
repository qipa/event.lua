local core = require "pathfinder.core"
local util = require "util"

local finder = core.create(101,"nav.tile")

local now = util.time()
for i = 1,10000 do
	finder:find(19,118,118,27)
end
print("time diff:",(util.time() - now) * 10)