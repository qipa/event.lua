local event = require "event"
local cjson = require "cjson"
local model = require "model"

local util = require "util"
local protocol = require "protocol"

local database_object = import "module.database_object"
local module_item_mgr = import "module.item_manager"
local task_manager = import "module.task_manager"


cls_agent_user = database_object.cls_database:inherit("agent_user","uid","cid")


function __init__(self)
	self.cls_agent_user:save_field("base_info")
	self.cls_agent_user:save_field("scene_info")
	self.cls_agent_user:save_field("item_mgr")
	self.cls_agent_user:save_field("task_mgr")
end


function cls_agent_user:create(cid,uid,account)
	self.cid = cid
	self.uid = uid
	self.account = account
	model.bind_agent_user_with_uid(uid,self)
	model.bind_agent_user_with_cid(cid,self)
end

function cls_agent_user:destroy()
	model.unbind_agent_user_with_uid(self.uid)
	model.unbind_agent_user_with_cid(self.cid)
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
	if not self.base_info then
		self.base_info = {uid = self.uid}
		self:dirty_field("base_info")
	end

	if not self.item_mgr then
		self.item_mgr = module_item_mgr.cls_item_mgr:new(self)
	end
	self.item_mgr:init(self)
	task_manager:init(self)
	
	self:fire_event("ENTER_GAME")
	self:send_client("s2c_agent_enter",{user_uid = self.uid})
	event.error(string.format("user:%d enter agent:%d",self.uid,env.dist_id))
end

function cls_agent_user:leave_game()
	self:fire_event("LEAVE_GAME")
	event.error(string.format("user:%d leave agent:%d",self.uid,env.dist_id))
end

function cls_agent_user:forward_world(message_id,message)
	local world_channel = model.get_world_channel()
	world_channel:send("handler.world_handler","forward",{uid = self.uid, message_id = message_id, message = message})
end

function cls_agent_user:forward_scene(message_id,message)
	self:send_scene("handler.scene_handler","forward",{uid = self.uid,message_id = message_id,message = message})
end

function cls_agent_user:send_scene(file,method,args)
	local scene_channel = self.scene_channel
	if not scene_channel then
		print(string.format("scene server:%d not connected",self.scene_server))
		return
	end
	scene_channel:send(file,method,args)
end

function cls_agent_user:sync_scene_info(scene_id,scene_uid,scene_server)
	self.scene_id = scene_id
	self.scene_uid = scene_uid
	self.scene_server = scene_server
	self.scene_channel = agent_server:get_scene_channel(scene_server)
end

function cls_agent_user:scene_down()
	self.scene_id = nil
	self.scene_uid = nil
	self.scene_server = nil
	self.scene_channel = nil
end
