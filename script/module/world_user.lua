local event = require "event"
local cjson = require "cjson"
local model = require "model"
local util = require "util"

local database_object = import "database_object"

cls_world_user = database_object.cls_database:inherit("world_user","uid")

function __init__(self)
	self.cls_world_user:save_field("world_user")
end


function cls_world_user:create(uid,cid,agent_server_id)
	self.uid = uid
	self.cid = cid
	self.agent_server_id = agent_server_id
	model.bind_world_user_with_uid(self.uid)
end

function cls_world_user:destroy()
	model.unbind_world_user_with_uid(self.uid)
end

function cls_world_user:db_index()
	return {uid = self.uid}
end

function cls_world_user:enter()

end

function cls_world_user:leave()

end

