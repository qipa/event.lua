local event = require "event"
local cjson = require "cjson"
local model = require "model"
local util = require "util"

local database_object = import "module.database_object"

cls_fighter = database_object.cls_database:inherit("fighter")

function __init__(self)
	
end

function cls_fighter:init(data)
	self.uid = data.uid
	self.scene_info = data.scene_info
	self.fighter_info = data.fighter_info
end

function cls_fighter:destroy()
	
end

function cls_fighter:do_enter(scene_uid)
	local scene = scene_server:get_scene(scene_uid)
	scene:enter(self)
end

function cls_fighter:do_leave()
	self.scene:leave(self)
end

function cls_fighter:object_enter(object)

end

function cls_fighter:object_leave(object)

end

function cls_fighter:find_path(from_x,from_z,to_x,to_z)
	return self.scene:find_path(from_x,from_z,to_x,to_z)
end

function cls_fighter:raycast(from_x,from_z,to_x,to_z)
	return self.scene:raycast(from_x,from_z,to_x,to_z)
end

function cls_fighter:pos_movable(x,z)
	return self.scene:pos_movable(x,z)
end

function cls_fighter:pos_around_movable(x,z,depth)
	return self.scene:pos_around_movable(x,z,depth)
end

function cls_fighter:move(x,z)
	self.scene:move(fighter,x,z)
	self.scene_info.x = x
	self.scene_info.z = z
end

function cls_fighter:set_pos(x,z)
	self:move(x,z)
end
