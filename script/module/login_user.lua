local model = require "model"
local object = import "object"
local server_handler = import "handler.server_handler"

cls_login_user = object.cls_base:inherit("login_user","account")

local PHASE = {
	LOGIN = 1,
	AGENT = 2,
	AGENT_LEAVING = 3,
}

function cls_login_user:create(cid,account)
	self.cid = cid
	self.phase = PHASE.LOGIN
	self.account = account
end

function cls_login_user:destroy()

end

function cls_login_user:leave(callback)
	if self.phase == PHASE.AGENT then
		local client_manager = model.get_client_manager()
		client_manager:close(self.cid)
		self.phase = PHASE.AGENT_LEAVING
		server_handler:send_agent(self.agent,"handler.agent_handler","user_kick",{uid = uid},function ()
			model.unbind_login_user_with_account(self.account)
			self:release()
			if callback then
				callback()
			end
		end)
		return
	elseif self.phase == PHASE.AGENT_LEAVING then
		return
	end
	model.unbind_login_user_with_account(self.account)
	self:release()
	if callback then
		callback()
	end
end

function cls_login_user:enter_agent(uid)

	local agent,agent_addr = server_handler:find_min_agent()
	self.uid = uid
	self.agent = agent
	self.phase = PHASE.AGENT
	server_handler:send_agent(agent,"handler.agent_handler","user_register",{uid = uid})
end

