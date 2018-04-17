local util = require "util"

local buidler = import "module.id_builder"
buidler:init(1)

util.time_diff("build id",function ()
	for i = 1,1024*1024 do
		buidler.alloc_user_uid()
	end
end)
