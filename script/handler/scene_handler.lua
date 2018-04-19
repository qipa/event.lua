local model = require "model"
local scene_user = import "module.scene_user"
local scene_server = import "module.scene_server"

function __init__(self)
	model.register_binder("fighter","uid")
end


function enter_scene(_,args)
	local fighter = scene_user.cls_scene_user:unpack(args.fighter)
	model.bind_fighter_with_uid(args.uid,fighter)
	fighter.user_uid = args.user_uid
	fighter.user_agent = args.user_agent

	local scene = scene_server:get_scene(args.scene_uid)
	scene:enter(fighter,args.pos)
end

function leave_scene(_,args)
	local fighter = model.fetch_fighter_with_uid(args.uid)
	model.unbind_fighter_with_uid(args.uid)

	local scene = scene_server:get_scene(fighter.scene_info.scene_uid)
	scene:leave(fighter)
	if args.switch == false then
		local db_channel = model.get_db_channel()
		fighter:save(db_channel)

		local updater = {}
		updater["$inc"] = {version = 1}
		updater["$set"] = {time = os.time()}
		db_channel:findAndModify("save_version",{query = {uid = user.uid},update = updater})
	end	
	fighter:release()
	return true
end

function create_scene(_,args)
	return scene_server:create_scene(args.scene_id)
end

function delete_scene(_,args)
	return scene_server:delete_scene(args.scene_uid)
end
