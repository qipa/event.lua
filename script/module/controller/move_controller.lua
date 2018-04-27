local util = require "util"
local vector2 = require "common.vector2"

local _M = {}

function new(fighter,x,z)
	local ctx = setmetatable({},{__index = _M})
	ctx.fighter = fighter
	ctx.x = x
	ctx.z = z
	ctx.active = true
	ctx.time = util.time()
	return ctx
end

function _M:update()
	if not self.active then
		return
	end

	local fighter = self.fighter

	local now = util.time()
	local interval = now - self.time
	local pos = fighter.scene_info.scene_pos
	local pass = (interval * fighter.speed) / 100

	local x,z = vector2.move_forward(pos.x,pos.z,self.x,self.z,pass)

	fighter.scene:fighter_move(fighter,x,z)
	
	pos.x = x
	pos.z = z

	self.time = now

	if vector2.distance(x,z,self.x,self.z) < 0.1 then
		fighter.move_ctrl = nil
	end
end

function _M:enable(val)
	self.active = val
end
