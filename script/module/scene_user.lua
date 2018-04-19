local event = require "event"
local cjson = require "cjson"
local model = require "model"
local util = require "util"
local protocol = require "protocol"

local database_object = import "module.database_object"
local scene_server = import "module.scene_server"
local server_manager = import "module.server_manager"

cls_scene_user = database_object.cls_database:inherit("scene_user","uid")

function __init__(self)
	self.cls_scene_user:save_field("fighter_info")
end


function cls_scene_user:create(uid,cid)
	self.uid = uid
end

function cls_scene_user:destroy()
	
end

function cls_scene_user:db_index()
	return {uid = self.uid}
end

function cls_scene_user:send_client(proto,args)
	local message_id,data = protocol.encode[proto](args)
	self:send_agent("handler.agent_handler","forward_client",{user_uid = self.user_uid,message_id = message_id,data = data})
end

function cls_scene_user:send_agent(file,method,args,callback)
	local agent_channel = server_manager:get_agent_channel(self.user_agent)
	agent_channel:send(file,method,args,callback)
end

function cls_scene_user:send_world(file,method,args,callback)
	local world_channel = model.get_world_channel()
	world_channel:send(file,method,args,callback)
end

function cls_scene_user:do_enter(scene_uid)
	local scene = scene_server:get_scene(scene_uid)
	scene:enter(self)
	self.scene_info.scene_uid = scene_uid
	self:dirty_field("scene_info")
end

function cls_scene_user:do_leave()
	local scene_uid = self.scene_info.scene_uid
	local scene = scene_server:get_scene(scene_uid)
	scene:leave(self)
	self.scene_info.scene_uid = 0
	self:dirty_field("scene_info")
end

