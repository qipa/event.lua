local object = import "object"

cls_collection = object.cls_base:inherit("database_collection")

function __init__()
	self.cls_collection:save_field("__name")
end

function cls_collection:save_data()
	local result = {}
	for field in pairs(self.__save_fields) do
		local data = self[field]
		if data then
			if data.save_data then
				result[field] = data:save_data()
			else
				result[field] = data
			end
		end
	end
	return result
end
