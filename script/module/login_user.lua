local event = require "event"
local cjson = require "cjson"
local model = require "model"
local util = require "util"
local protocol = require "protocol"

local id_builder = import "module.id_builder"
local database_object = import "module.database_object"
local server_manager = import "module.server_manager"

cls_login_user = database_object.cls_database:inherit("login_user","account")

function __init__(self)
	self.cls_login_user:save_field("role_list")
end

local PHASE = {
	LOADING = 1,
	LOGIN = 2,
}

function cls_login_user:create(cid,account)
	self.cid = cid
	self.phase = PHASE.LOADING
	self.account = account
	model.bind_login_user_with_account(account,self)
end

function cls_login_user:db_index()
	return {account = self.account}
end

function cls_login_user:send_client(proto,args)
	local client_manager = model.get_client_manager()
	local message_id,data = protocol.encode[proto](args)
	client_manager:send(self.cid,message_id,data)
end

function cls_login_user:auth()
	local db_channel = model.get_db_channel()
	
	self.phase = PHASE.LOADING
	self:load(db_channel)
	self.phase = PHASE.LOGIN

	local self = model.fetch_login_user_with_account(self.account)
	if not self then
		return
	end

	if not self.role_list then
		self.role_list = {list = {}}
		self:dirty_field("role_list")
	end

	local result = {}
	for _,role in pairs(self.role_list.list) do
		table.insert(result,{uid = role.uid,name = role.name})
	end

	self:send_client("s2c_login_auth",{list = result})
end

function cls_login_user:create_role(career)
	local role = {career = career,name = "mrq",uid = id_builder:alloc_user_uid()}
	table.insert(self.role_list.list,role)
	self:dirty_field("role_list")

	local result = {}
	for _,role in pairs(self.role_list.list) do
		table.insert(result,{uid = role.uid,name = role.name})
	end

	self:send_client("s2c_create_role",{list = result})
end

function cls_login_user:delete_role(uid)

end

function cls_login_user:create_name(name)

end

function cls_login_user:random_name()

end

function cls_login_user:destroy()
	model.unbind_login_user_with_account(self.account)
end

function cls_login_user:leave()
	local db_channel = model.get_db_channel()
	self:save(db_channel)
	self:release()
end

function cls_login_user:enter_agent(uid)
	local agent_server,agent_addr = server_manager:find_min_agent()
	local time = util.time()
	local json = cjson.encode({account = self.account,uid = uid})
	local token = util.authcode(json,tostring(time),1)

	server_manager:send_agent(agent_server,"handler.agent_handler","user_register",{token = token,time = time,uid = uid,account = self.account})
	
	self:send_client("s2c_login_enter",{token = token,ip = agent_addr.ip,port = agent_addr.port})

	return agent_server
end

