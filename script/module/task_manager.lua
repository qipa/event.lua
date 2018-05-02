local event = require "event"
local cjson = require "cjson"
local model = require "model"
local util = require "util"

local database_collection = import "module.database_collection"


cls_task_mgr = database_collection.cls_collection:inherit("task_mgr")

function __init__(self)
	self.cls_task_mgr:save_field("uid")
	self.cls_task_mgr:save_field("info")
end

function cls_task_mgr:create(uid)
	self.uid = uid
	self.info = {}
end

function cls_task_mgr:init()
	
end

function cls_task_mgr:destroy()

end