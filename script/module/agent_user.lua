local event = require "event"
local cjson = require "cjson"
local model = require "model"
local channel = require "channel"
local util = require "util"
local protocol = require "protocol"

local database_object = import "module.database_object"
local module_item_mgr = import "module.item_manager"

_scene_channel_ctx = _scene_channel_ctx or {}

local scene_channel = channel:inherit()

function scene_channel:disconnect()
	_scene_channel_ctx[self.id] = nil
end

cls_agent_user = database_object.cls_database:inherit("agent_user","uid","cid")


function __init__(self)
	self.cls_agent_user:save_field("user_info")
	self.cls_agent_user:save_field("scene_info")
end


function cls_agent_user:create(cid,uid,account)
	self.cid = cid
	self.uid = uid
	self.account = account
	model.bind_agent_user_with_uid(uid,self)
	model.bind_agent_user_with_cid(cid,self)
end

function cls_agent_user:destroy()
	model.unbind_agent_user_with_uid(self.uid,self)
	model.unbind_agent_user_with_cid(self.cid,self)
end

function cls_agent_user:db_index()
	return {uid = self.uid}
end

function cls_agent_user:send_client(proto,args)
	local message_id,data = protocol.encode[proto](args)
	self:forward_client(message_id,data)
end

function cls_agent_user:forward_client(message_id,data)
	local client_manager = model.get_client_manager()
	client_manager:send(self.cid,message_id,data)
end

function cls_agent_user:enter_game()
	local item_mgr = self.item_mgr
	if not item_mgr then
		item_mgr = module_item_mgr.cls_item_mgr:new(self.uid)
	end
	self:send_client("s2c_agent_enter",{user_uid = self.uid})
	event.error(string.format("user:%d enter agent:%d",self.uid,env.dist_id))
end

function cls_agent_user:leave_game()
	event.error(string.format("user:%d leave agent:%d",self.uid,env.dist_id))
end

function cls_agent_user:connect_scene_server(scene_server,scene_addr)
	local scene_channel = _scene_channel_ctx[scene_server]
	if scene_channel then
		return true
	end
	local addr
	if scene_addr.file then
		addr = string.format("ipc://%s",scene_addr.file)
	else
		addr = string.format("tcp://%s:%d",scene_addr.ip,scene_addr.port)
	end

	local channel,reason = event.connect(addr,4,false,scene_channel)
	if not channel then
		event.error(string.format("connect scene server:%d faield:%s",addr,reason))
		return false
	end
	_scene_channel_ctx[scene_server] = channel
	return true
end

function cls_agent_user:forward_world(message_id,message)
	local world_channel = model.get_world_channel()
	world_channel:send("handler.world_handler","forward",{uid = self.uid, message_id = message_id, message = message})
end

function cls_agent_user:forward_scene(message_id,message)
	self:send_scene("handler.scene_handler","forward",{uid = self.uid,message_id = message_id,message = message})
end

function cls_agent_user:send_scene(file,method,args)
	local scene_channel = _scene_channel_ctx[self.scene_server]
	if not scene_channel then
		print(string.format("scene server:%d not connected",self.scene_server))
		return
	end
	scene_channel:send(file,method,args)
end
