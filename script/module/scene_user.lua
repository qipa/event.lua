local event = require "event"
local cjson = require "cjson"
local model = require "model"
local util = require "util"
local protocol = require "protocol"

local fighter = import "module.fighter"
local server_manager = import "module.server_manager"
local scene_server = import "module.scene_server"

cls_scene_user = fighter.cls_fighter:inherit("scene_user","uid")

function __init__(self)
	self.cls_scene_user:save_field("base_info")
	self.cls_scene_user:save_field("fighter_info")
	self.cls_scene_user:save_field("scene_info")
end

function cls_scene_user:create(uid)
	self.uid = uid
end

function cls_scene_user:destroy()
	
end

function cls_scene_user:db_index()
	return {uid = self.uid}
end

function cls_scene_user:send_client(proto,args)
	local message_id,data = protocol.encode[proto](args)
	self:send_agent("handler.agent_handler","forward_client",{uid = self.uid,message_id = message_id,data = data})
end

function cls_scene_user:send_agent(file,method,args,callback)
	local agent_channel = server_manager:get_agent_channel(self.user_agent)
	agent_channel:send(file,method,args,callback)
end

function cls_scene_user:send_world(file,method,args,callback)
	local world_channel = model.get_world_channel()
	world_channel:send(file,method,args,callback)
end

function cls_scene_user:do_enter(scene_id,scene_uid,aoi_id)
	fighter.cls_fighter.do_enter(self,scene_id,scene_uid,aoi_id)
	self:dirty_field("scene_info")
	self:send_agent("handler.agent_handler","sync_scene_info",{user_uid = self.uid,scene_server = env.dist_id})
	self:send_client("s2c_scene_enter",{scene_id = scene_id,scene_uid = scene_uid})
end

function cls_scene_user:do_leave()
	fighter.cls_fighter.do_leave(self)
	self:dirty_field("scene_info")
end

function cls_scene_user:transfer_scene(scene_id,scene_uid,x,z)
	local scene = scene_server:get_scene(scene_uid)
	if scene and self.scene == scene then
		self:set_pos(x,z)
		return
	end
	scene_server:launch_transfer_scene(self,scene_id,scene_uid,x,z)
end
