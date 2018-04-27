local event = require "event"
local cjson = require "cjson"
local model = require "model"
local util = require "util"

local database_object = import "module.database_object"
local scene_server = import "module.scene_server"
local move_controller = import "module.move_controller"
cls_fighter = database_object.cls_database:inherit("fighter")

function __init__(self)
	self.cls_fighter:pack_field("uid")
	self.cls_fighter:pack_field("scene_info")
	self.cls_fighter:pack_field("attr")
	self.cls_fighter:pack_field("agent_user")
end

function cls_fighter:init(data)
	-- self.uid = data.uid
	-- self.scene_info = data.scene_info
	-- self.fighter_info = data.fighter_info
	self.view_list = {}
	self.move_ctrl = nil
end

function cls_fighter:destroy()
	
end

function cls_fighter:do_enter(scene_id,scene_uid,aoi_id)
	event.error(string.format("user:%d enter scene:%d %d %d %d",self.uid,env.dist_id,scene_id,scene_uid,aoi_id))
	local scene = scene_server:get_scene(scene_uid)
	self.scene = scene
	self.scene_info.scene_uid = scene_uid
	self.aoi_id = aoi_id
end

function cls_fighter:do_leave()
	event.error(string.format("user:%d leave scene:%d %d %d",self.uid,env.dist_id,self.scene_info.scene_id,self.scene_info.scene_uid))
	self.scene = nil
	self.scene_info.scene_uid = nil
end

function cls_fighter:object_enter(objs)
	for _,fighter in pairs(objs) do
		self.view_list[fighter.uid] = false
	end
end

function cls_fighter:object_leave(objs)
	for _,fighter in pairs(objs) do
		self.view_list[fighter.uid] = nil
	end
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
	if self.move_ctrl then
		self.move_ctrl:disable()
		self.move_ctrl = nil
	end
	self.move_ctrl = move_controller.new(self,self.speed,x,z)
end

function cls_fighter:set_pos(x,z)
	self:move(x,z)
end

function cls_fighter:update()
	-- print(self.uid,"update")
	if self.move_ctrl then
		self.move_ctrl:update()
	end
end
