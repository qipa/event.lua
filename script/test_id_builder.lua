local util = require "util"
env.dist_id = 100
local buidler = import "module.id_builder"
buidler:init(100)

util.time_diff(function ()
	for i = 1,1024*1024 do
		buidler.alloc_user_uid()
	end
end)
