local util = require "util"
local vector2 = require "vector2"

local _M = {}

function new(fighter,x,z)
	local ctx = setmetatable({fighter = fighter,x = x,z = z,active = true,time = util.time()},{__index = _M})
	return ctx
end

function _M:update()
	if not self.active then
		return
	end

	local fighter = self.fighter

	local now = util.time()
	local interval = now - self.time

	local pass = interval * fighter.speed

	local x,z = vector2.move_forward(fighter.scene_info.x,fighter.scene_info.z,self.x,self.z,pass)

	fighter.scene:fighter_move(self,x,z)
	
	fighter.scene_info.x = x
	fighter.scene_info.z = z

	self.time = now

	if vector2.distance(x,z,self.x,self.z) < 0.1 then
		fighter.move_ctrl = nil
	end
end

function _M:enable(val)
	self.active = val
end
