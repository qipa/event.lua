local scene_user = import "module.scene_user"
local item_mgr = import "module.item_manager"


local fighter = scene_user.cls_scene_user:new(1)

if not fighter.attr then
	fighter.attr = {atk = 100,def = 100}
end
if not fighter.scene_info then
	fighter.scene_info = {scene_id = 1001,scene_pos = {x = 100,z = 100}}
end

if not fighter.fighter_info then
	fighter.fighter_info = {scene_id = 1001,scene_pos = {x = 100,z = 100}}
end

fighter.item_mgr = item_mgr.cls_item_mgr:new(fighter)

fighter.fuck_info = {fuck = "mrq"}

-- table.print(fighter,"fighter")

local str = table.tostring(fighter)

table.print(table.decode(str))