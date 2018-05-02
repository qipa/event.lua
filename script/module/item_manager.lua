local event = require "event"
local cjson = require "cjson"
local model = require "model"
local util = require "util"

local database_collection = import "module.database_collection"
local module_item = import "module.item.item"


cls_item_mgr = database_collection.cls_collection:inherit("item_mgr")

function __init__(self)
	self.cls_item_mgr:save_field("uid")
	self.cls_item_mgr:save_field("mgr")
end

function cls_item_mgr:create(uid)
	self.uid = uid
	self.mgr = {}

	for i = 1,10 do
		local item = module_item.cls_item_base:new(i,i)
		self.mgr[tostring(item.uid)] = item
	end
end

function cls_item_mgr:init()

end

function cls_item_mgr:destroy()

end