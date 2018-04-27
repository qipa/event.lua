local util = require "util"
local vector2 = require "common.vector2"

local _M = {}

function new(fighter,ai_id)
	local ctx = setmetatable({},{__index = _M})
	ctx.fighter = fighter
	ctx.ai_id = ai_id
	ctx.active = true
	ctx.time = util.time()
	return ctx
end

function _M:update()
	if not self.active then
		return
	end

	local fighter = self.fighter
end

function _M:enable(val)
	self.active = val
end
