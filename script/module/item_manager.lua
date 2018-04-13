local event = require "event"
local cjson = require "cjson"
local model = require "model"
local util = require "util"

local database_collection = import "module.database_collection"


cls_item_mgr = database_collection.cls_collection:inherit("item_mgr")

function cls_item_mgr:create()

end

function cls_item_mgr:init()

end

function cls_item_mgr:destroy()

end