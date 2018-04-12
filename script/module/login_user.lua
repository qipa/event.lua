local model = require "model"
local object = import "object"

cls_login_user = object.cls_base:inherit("login_user","account")

local PHASE = {
	LOGIN_ENTER = 1,
	AGENT_REGISTER = 2,
	AGENT_ENTER = 3,
	LOGIN_LEAVE_PREPARE = 4,
	LOGIN_LEAVE_DONE = 5
}

function cls_login_user:create(cid,account)
	self.cid = cid
	self.phase = PHASE.LOGIN_ENTER
	self.account = account
end

function cls_login_user:destroy()

end

function cls_login_user:leave()
	model.unbind_login_user_with_account(self.account)
	self:release()
	if self.phase == PHASE.AGENT_REGISTER or self.phase == PHASE.AGENT_ENTER then

	end
end

function cls_login_user:register_agent()

end

