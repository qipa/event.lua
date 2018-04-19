local event = require "event"
local cjson = require "cjson"
local model = require "model"
local util = require "util"

local database_object = import "module.database_object"

cls_world_user = database_object.cls_database:inherit("world_user","uid")

function __init__(self)
	self.cls_world_user:save_field("world_user")
end


function cls_world_user:create(uid,user_agent)
	self.uid = uid
	self.user_agent = user_agent
	self.loading = false
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

