local core = require "pathfinder.core"
local util = require "util"

local finder = core.create(101,"nav.tile")

util.time_diff(function ()
	for i = 1,1000 do
		finder:find(22,109,124,18)
	end

end)
