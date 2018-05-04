local object = import "module.object"

--对应着mongodb中的文档(document)概念
cls_document = object.cls_base:inherit("database_collection")

function __init__(self)
	self.cls_document:save_field("__name")
end

local function clone_table(data)
	depth = depth + 1
	local result = {}
	for k,v in pairs(data) do
		if type(v) == "table" then
			if v.save_data then
				result[k] = v:save_data(depth)
			else
				result[k] = clone_table(v,depth)
			end
		else
			result[k] = v
		end
	end
	return result
end

function cls_document:save_data(depth)
	depth = depth + 1
	local result = {}
	for field in pairs(self.__save_fields) do
		local data = self[field]
		if data then
			if type(data) == "table" then
				if data.save_data then
					result[field] = data:save_data(depth)
				else
					result[field] = clone_table(data,depth)
				end
			else
				result[field] = data
			end
		end
	end
	return result
end
