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

function update()
	for scene_uid,scene in pairs(_scene_ctx) do
		scene:update()
	end
end