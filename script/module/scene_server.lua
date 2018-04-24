local event = require "event"
local model = require "model"

local scene = import "module.scene"
local scene_user = import "module.scene_user"
local id_builder = import "module.id_builder"

_scene_ctx = _scene_ctx or {}
_scene_uid2id = _scene_uid2id or {}

function __init__(self)
	self.timer = event.timer(0.1,function ()
		self:update()
	end)
	
	self.db_timer = event.timer(30,function ()
		local db_channel = model.get_db_channel()
		local all = model.fetch_scene_user()
		for _,fighter in pairs(all) do
			fighter:save(db_channel)
		end
	end)

end

function stop()
	local db_channel = model.get_db_channel()
	local all = model.fetch_scene_user()
	for _,fighter in pairs(all) do
		fighter:save(db_channel)
	end
end

function create_scene(self,scene_id)
	local scene_info = _scene_ctx[scene_id]
	if not scene_info then
		scene_info = {}
		_scene_ctx[scene_id] = scene_info
	end

	local scene_uid = id_builder:alloc_scene_uid()
	local scene = scene.cls_scene:new(scene_id,scene_uid)
	scene_info[scene_uid] = scene
	_scene_uid2id[scene_uid] = scene_id
	return scene_uid
end

function delete_scene(self,scene_uid)
	local scene_id = _scene_uid2id[scene_uid]
	if not scene_id then
		return
	end

	local scene_info = _scene_ctx[scene_id]
	local scene = scene_info[scene_uid]
	scene:release()
	scene_info[scene_uid] = nil
	_scene_uid2id[scene_uid] = nil
end

function get_scene(self,scene_uid)
	local scene_id = _scene_uid2id[scene_uid]
	if not scene_id then
		return
	end
	local scene_info = _scene_ctx[scene_id]
	return scene_info[scene_uid]
end

function enter_scene(self,fighter_data,user_agent,scene_uid,pos)
	local fighter = scene_user.cls_scene_user:unpack(fighter_data)
	fighter.user_agent = user_agent
	print("user_agent",user_agent)
	model.bind_scene_user_with_uid(fighter.uid,fighter)

	local scene = self:get_scene(scene_uid)
	assert(scene ~= nil,scene_uid)
	scene:enter(fighter,pos)
end

function leave_scene(self,user_uid,switch)
	local fighter = model.fetch_scene_user_with_uid(user_uid)
	model.unbind_scene_user_with_uid(user_uid)

	local scene = self:get_scene(fighter.scene_info.scene_uid)
	scene:leave(fighter)

	local db_channel = model.get_db_channel()
	fighter:save(db_channel)

	local updater = {}
	updater["$inc"] = {version = 1}
	updater["$set"] = {time = os.time()}
	db_channel:findAndModify("save_version",{query = {uid = fighter.uid},update = updater,upsert = true})

	local fighter_data
	if switch then
		fighter_data = fighter:pack()
	end
	
	fighter:release()

	return fighter_data
end


function transfer_scene(self,fighter,scene_id,scene_uid,x,z)
	local world_channel = model.get_world_channel()
	world_channel:send("module.scene_manager","transfer_scene",{scene_id = scene_id,scene_uid = scene_uid,pos = {x = x,z = z},fighter = fighter:pack()})
end

function update()
	for _,scene_info in pairs(_scene_ctx) do
		for _,scene in pairs(scene_info) do
			scene:update()
		end
	end
end