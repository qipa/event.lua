local util = require "util"

local _M = {}

function new(fighter,x,z)
	local ctx = setmetatable({fighter = fighter,x = x,z = z,enable = true,time = util.time()},{__index = _M})
	return ctx
end

function _M:update()

	if not self.enable then
		return
	end
	local now = util.time()
	local interval = now - self.time
	local distance = interval * self.fighter.speed

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
