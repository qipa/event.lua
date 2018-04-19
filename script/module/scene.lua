local aoi_core = require "fastaoi.core"
local nav_core = require "nav.core"
local object = import "module.object"

cls_scene = object.cls_base:inherit("scene")

function cls_scene:create(scene_id,scene_uid)
	self.scene_id = scene_id
	self.scene_uid = scene_uid
	self.fighter_ctx = {}
	self.aoi = aoi_core.new(1000,1000,2,5,1000)
	self.nav = nav_core.create(scene_id)
end

function cls_scene:enter(fighter,pos)
	fighter:do_enter(self.scene_id,self.scene_uid)
	self.fighter_ctx[fighter.uid] = fighter
	local set = self.aoi:enter(fighter.uid,fighter.scene_info.pos.x,fighter.scene_info.pos.z,0)

	for _,uid in pairs(set) do
		local other = self.fighter_ctx[uid]
		fighter:object_enter(other)
		other:object_enter(fighter)
	end
end

function cls_scene:leave(fighter)
	fighter:do_leave()
	local set = self.aoi:leave(fighter.uid)
	for _,uid in pairs(set) do
		local other = self.fighter_ctx[uid]
		other:object_leave(fighter)
	end

	self.fighter_ctx[fighter.uid] = nil
end

function cls_scene:find_path(from_x,from_z,to_x,to_z)
	return self.nav:find(from_x,from_z,to_x,to_z)
end

function cls_scene:raycast(from_x,from_z,to_x,to_z)
	return self.nav:raycast(from_x,from_z,to_x,to_z)
end

function cls_scene:pos_movable(x,z)
	return self.nav:movable(x,z)
end

function cls_scene:pos_around_movable(x,z,depth)
	return self.nav:around_movable(x,z,depth)
end

function cls_scene:fighter_move(fighter,x,z)
	local enter,_,leave,_ = self.aoi:update(fighter.uid,x,z)
	
	for _,uid in pairs(enter) do
		local other = self.fighter_ctx[uid]
		fighter:object_enter(other)
		other:object_enter(fighter)
	end

	for _,uid in pairs(leave) do
		local other = self.fighter_ctx[uid]
		other:object_leave(fighter)
	end
end

function cls_scene:update(now)
	for _,fighter in pairs(self.fighter_ctx) do
		fighter:update()
	end

end