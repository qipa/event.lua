local object = import "module.object"

--对应着mongodb中的文档(document)概念
cls_document = object.cls_base:inherit("database_document")

function __init__(self)
	self.cls_document:save_field("__name")
end

function cls_document:save_data()
	local result = {}
	for field in pairs(self.__save_fields) do
		local data = self[field]
		if data then
			result[field] = data
		end
	end
	return result
end
