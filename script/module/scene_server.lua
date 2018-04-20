local event = require "event"
local model = require "model"

local scene = import "module.scene"
local scene_user = import "module.scene_user"
local id_builder = import "module.id_builder"

_scene_ctx = _scene_ctx or {}

function __init__(self)
	self.timer = event.timer(0.1,function ()
		self:update()
	end)
	
	self.db_timer = event.timer(30,function ()
		local db_channel = model.get_db_channel()
		local all = model.fetch_fighter()
		for _,fighter in pairs(all) do
			fighter:save(db_channel)
		end
	end)

end

function stop()
	local db_channel = model.get_db_channel()
	local all = model.fetch_fighter()
	for _,fighter in pairs(all) do
		fighter:save(db_channel)
	end
end

function create_scene(self,scene_id)
	local scene_uid = id_builder:alloc_scene_uid()
	local scene = scene.cls_scene:new(scene_id,scene_uid)
	_scene_ctx[scene_uid] = scene
	return scene_uid
end

function delete_scene(self,scene_uid)
	local scene = _scene_ctx[scene_uid]
	scene:release()
	_scene_ctx[scene_uid] = nil
end

function get_scene(self,scene_uid)
	return _scene_ctx[scene_uid]
end

function enter_scene(self,fighter_data,scene_uid,pos)
	local fighter = scene_user.cls_scene_user:unpack(fighter_data)
	model.bind_scene_user_with_uid(fighter.uid,fighter)

	local scene = self:get_scene(scene_uid)
	scene:enter(fighter,pos)
end

function leave_scene(self,user_uid)
	local fighter = model.fetch_fighter_with_uid(user_uid)
	model.unbind_scene_user_with_uid(user_uid)

	local scene = self:get_scene(fighter.scene_info.scene_uid)
	scene:leave(fighter)

	local fighter_data = fighter:pack()

	local db_channel = model.get_db_channel()
	fighter:save(db_channel)

	local updater = {}
	updater["$inc"] = {version = 1}
	updater["$set"] = {time = os.time()}
	db_channel:findAndModify("save_version",{query = {uid = user.uid},update = updater})
	
	fighter:release()

	return fighter_data
end


function transfer_scene(self,fighter,scene_id,scene_uid,x,z)
	local master_channel = model.get_master_channel()
	master_channel:send("handler.master_handler","transfer_scene",{scene_id = scene_id,scene_uid = scene_uid,pos = {x = x,z = z},fighter = fighter:pack()})
end

function update()
	for scene_uid,scene in pairs(_scene_ctx) do
		scene:update()
	end
end