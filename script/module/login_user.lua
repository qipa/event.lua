local event = require "event"
local model = require "model"
local object = import "object"
local server_handler = import "handler.server_handler"

cls_login_user = object.cls_base:inherit("login_user","account")

function __init__(self)
	self.cls_login_user:save_field("login_info")
end

local PHASE = {
	LOGIN = 1,
	AGENT = 2,
	AGENT_LEAVING = 3,
}

function cls_login_user:create(cid,account)
	self.cid = cid
	self.phase = PHASE.LOGIN
	self.account = account
	self.timer = event.timer(1,function (timer)
		local user = model.fetch_login_user_with_account(account)
		if not user then
			return
		end
		local db_channel = model.get_mongod
		user:save(db_channel)
	end)
end

function cls_login_user:db_index()
	return {account = self.account}
end

function cls_login_user:auth()
	local db_channel = model.get_mongodb()
	self:load(db_channel)
	local self = model.fetch_login_user_with_account(self.account)
	if not self then
		return
	end
end

function cls_login_user:create_role(career,name)

end

function cls_login_user:delete_role(uid)

end

function cls_login_user:create_name(name)

end

function cls_login_user:random_name()

end

function cls_login_user:destroy()
	self.timer:cancel()
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

