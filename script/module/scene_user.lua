local event = require "event"
local cjson = require "cjson"
local model = require "model"
local util = require "util"

local database_object = import "module.database_object"
local scene_server = import "module.scene_server"

cls_scene_user = database_object.cls_database:inherit("scene_user","uid")

function __init__(self)
	self.cls_scene_user:save_field("fighter_info")
end


function cls_scene_user:create(uid,cid,agent_server_id)
	self.uid = uid
end

function cls_scene_user:destroy()
	
end

function cls_scene_user:db_index()
	return {uid = self.uid}
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

