local event = require "event"
local cjson = require "cjson"
local model = require "model"
local util = require "util"

local database_object = import "database_object"
local server_handler = import "handler.server_handler"

cls_login_user = database_object.cls_database:inherit("login_user","account")

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
		local db_channel = model.get_mongod()
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
	model.unbind_login_user_with_account(self.account)
end

function cls_login_user:leave(callback)
	if self.phase == PHASE.AGENT then
		local client_manager = model.get_client_manager()
		client_manager:close(self.cid)
		self.phase = PHASE.AGENT_LEAVING
		server_handler:send_agent(self.agent,"handler.agent_handler","user_kick",{uid = uid},function ()
			self:release()
			if callback then
				callback()
			end
		end)
		return
	elseif self.phase == PHASE.AGENT_LEAVING then
		return
	end
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

	local time = util.time()
	local json = cjson.encode({account = self.account,uid = uid})
	local token = util.authcode(json,tostring(time),1)
	server_handler:send_agent(agent,"handler.agent_handler","user_register",{token = token,time = time})
end

