local event = require "event"
local cjson = require "cjson"
local model = require "model"
local util = require "util"

local database_object = import "database_object"

cls_scene_user = database_object.cls_database:inherit("scene_user","uid")

function __init__(self)
	self.cls_scene_user:save_field("fighter_info")
end


function cls_scene_user:create(uid,cid,agent_server_id)
	self.uid = uid
	self.cid = cid
	self.agent_server_id = agent_server_id
	model.bind_scene_user_with_uid(self.uid)
end

function cls_scene_user:destroy()
	model.unbind_scene_user_with_uid(self.uid)
end

function cls_scene_user:db_index()
	return {uid = self.uid}
end

function cls_scene_user:enter()

end

function cls_scene_user:leave()

end

