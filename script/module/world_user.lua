local event = require "event"
local cjson = require "cjson"
local model = require "model"
local util = require "util"
local protocol = require "protocol"
local server_manager = import "module.server_manager"
local database_object = import "module.database_object"

cls_world_user = database_object.cls_database:inherit("world_user","uid")

function __init__(self)
	self.cls_world_user:save_field("world_user")
end


function cls_world_user:create(uid,user_agent)
	self.uid = uid
	self.user_agent = user_agent
	self.loading = false
	model.bind_world_user_with_uid(self.uid,self)
end

function cls_world_user:destroy()
	model.unbind_world_user_with_uid(self.uid)
end

function cls_world_user:db_index()
	return {uid = self.uid}
end

function cls_world_user:send_client(proto,args)
	local message_id,data = protocol.encode[proto](args)
	self:send_agent("handler.agent_handler","forward_client",{uid = self.uid,message_id = message_id,data = data})
end

function cls_world_user:send_agent(file,method,args,callback)
	local agent_channel = server_manager:get_agent_channel(self.user_agent)
	agent_channel:send(file,method,args,callback)
end

function cls_world_user:enter()
	event.error(string.format("user:%d enter world:%d",self.uid,env.dist_id))
	self:send_client("s2c_world_enter",{user_uid = self.uid})
end

function cls_world_user:leave()
	event.error(string.format("user:%d leave world:%d",self.uid,env.dist_id))
end

