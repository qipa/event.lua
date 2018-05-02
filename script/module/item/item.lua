local id_builder = import "module.id_builder"
local database_collection = import "module.database_collection"


cls_item_base = database_collection.cls_collection:inherit("item_base")

function __init__(self)
	self.cls_item_base:save_field("uid")
	self.cls_item_base:save_field("cid")
	self.cls_item_base:save_field("amount")
end

function cls_item_base:create(cid,amount)
	self.cid = cid
	self.amount = amount
	self.uid = id_builder:alloc_item_uid()
end

function cls_item_base:init()

end

function cls_item_base:destroy()

end