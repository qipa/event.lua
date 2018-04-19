local model = require "model"
local world_server = import "module.world_server"


function enter_world(channel,args)
	world_server:enter(args.user_uid,args.user_agent)
end

function leave_world(channel,args)
	return world_server:leave(args.user_uid)
end