local model = require "model"
local route = require "route"
local protocol = require "protocol"
local scene_user = import "module.scene_user"
local scene_server = import "module.scene_server"

function __init__(self)
	protocol.handler["c2s_move"] = req_move
end


function enter_scene(_,args)
	scene_server:enter_scene(args.fighter_data,args.user_agent,args.scene_uid,args.pos)
end

function leave_scene(_,args)
	return scene_server:leave_scene(args.user_uid,args.switch)
end

function create_scene(_,args)
	return scene_server:create_scene(args.scene_id)
end

function delete_scene(_,args)
	return scene_server:delete_scene(args.scene_uid)
end


function forward(_,args)
	local user = model.fetch_scene_user_with_uid(args.uid)
	if not user then
		return
	else
		route.dispatch_client(args.message_id,args.message,nil,user)
	end
end

function req_move(user,args)
	user:move(args.x,args.z)
end