local id_builder = import "module.id_builder"
local module_item = import "module.item.item"


cls_equipment = module_item.cls_item_base:inherit("equipment")


function cls_equipment:create(cid,amount)
	self.cid = cid
	self.amount = amount
	self.uid = id_builder:alloc_item_uid()
end

function cls_equipment:init()

end

function cls_equipment:destroy()

end