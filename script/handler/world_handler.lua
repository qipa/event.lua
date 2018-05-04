local model = require "model"
local world_server = import "module.world_server"


function enter_world(channel,args)
	world_server:enter(args.user_uid,args.user_agent)
end

function leave_world(channel,args)
	return world_server:leave(args.user_uid)
end

function sync_scene_info(_,args)
	local user = model.fetch_world_user_with_uid(args.user_uid)
	if not user then
		return
	end
	user:sync_scene_info(args.scene_server,args.scene_id,args.scene_uid)
end

function forward(_,args)
	local user = model.fetch_world_user_with_uid(args.uid)
	if not user then
		return
	else
		route.dispatch_client(args.message_id,args.message,nil,user)
	end
end

function server_stop(_,args)
	world_server:server_stop(args.id)
end
