local id_builder = import "module.id_builder"
local database_document = import "module.database_document"


cls_item_base = database_document.cls_document:inherit("item_base")

function __init__(self)
	self.cls_item_base:save_field("uid")
	self.cls_item_base:save_field("cid")
	self.cls_item_base:save_field("amount")
end

local id = 1
function cls_item_base:create(cid,amount)
	self.cid = cid
	self.amount = amount
	self.uid = id
	id = id + 1
end

function cls_item_base:init()

end

function cls_item_base:destroy()

end

function cls_item_base:get_info()
	return {cid = self.cid,uid = self.uid,amount = self.amount}
end
