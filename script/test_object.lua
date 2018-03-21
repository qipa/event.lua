local object = import "object"
local object_mgr = import "object_manager"

local cls_fuck = object.cls_base:inherit("fuck","id")

function cls_fuck:create(...)
	print("cls_fuck:create",...)
end

local obj = cls_fuck:new("fuck","lua")


print(obj)
table.print(object_mgr)