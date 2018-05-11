local bson = require "bson"
local util = require "util"
local scene_user = import "module.scene_user"
local item_mgr = import "module.item_manager"
local id_builder = import "module.id_builder"


_G.config = {}
print(loadfile("./config/item.lua"))
	local data = loadfile("./config/item.lua")()
	_G.config["item"] = data


local fighter = scene_user.cls_scene_user:new(1)

if not fighter.attr then
	fighter.attr = {atk = 100,def = 100}
	fighter:dirty_field("attr")
end
if not fighter.scene_info then
	fighter.scene_info = {scene_id = 1001,scene_pos = {x = 100,z = 100}}
	fighter:dirty_field("scene_info")
end

if not fighter.fighter_info then
	fighter.fighter_info = {scene_id = 1001,scene_pos = {x = 100,z = 100}}
	fighter:dirty_field("fighter_info")
end

fighter.item_mgr = item_mgr.cls_item_mgr:new(fighter)

fighter.item_mgr:insert_item_by_cid(100,1)

fighter.fuck_info = {fuck = "mrq"}

-- table.print(fighter,"fighter")

fighter:save()

