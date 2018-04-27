local util = require "util"
local vector2 = require "vector2"

local _M = {}

function new(fighter,x,z)
	local ctx = setmetatable({fighter = fighter,x = x,z = z,enable = true,time = util.time()},{__index = _M})
	return ctx
end

function _M:update()

	if not self.enable then
		return
	end
	local from_x,from_z = self.fighter.scene_info.x,self.fighter.scene_info.z

	local now = util.time()
	local interval = now - self.time

	local pass = interval * self.fighter.speed
	local distance = vector2.distance(from_x,from_z,x,z)
	local ratio = pass / distance
	if ratio > 1 then
		ratio = 1
	end

	local x,z = vector2.lerp(from_x,from_z,x,z)

	self.fighter.scene:fighter_move(self,x,z)
	
	self.fighter.scene_info.x = x
	self.fighter.scene_info.z = z

	self.time = now
	if x == self.x and z == self.z then
		self.fighter.move_ctrl = nil
	end
end

function _M:disable()
	self.enable = false
end
