local model = require "model"
local scene_user = import "module.scene_user"
local scene_server = import "module.scene_server"

function __init__(self)
	
end


function enter_scene(_,args)
	table.print(args,"enter_scene")
	scene_server:enter_scene(args.fighter_data,args.scene_uid,args.pos)
end

function leave_scene(_,args)
	return scene_server:leave_scene(args.uid,args.switch)
end

function create_scene(_,args)
	return scene_server:create_scene(args.scene_id)
end

function delete_scene(_,args)
	return scene_server:delete_scene(args.scene_uid)
end
