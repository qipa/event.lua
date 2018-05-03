local object = import "module.object"

cls_table = object.cls_base:inherit("database_collection")

function __init__(self)
	self.cls_table:save_field("__name")
end

local function clone_table(data)
	local result = {}
	for k,v in pairs(data) do
		if type(v) == "table" then
			if v.save_data then
				result[k] = v:save_data()
			else
				result[k] = clone_table(v)
			end
		else
			result[k] = v
		end
	end
	return result
end

function cls_table:save_data(root)
	local result = {}
	for field,have_object in pairs(self.__save_fields) do
		local data = self[field]
		if data then
			if type(data) == "table" then
				if data.save_data then
					result[field] = data:save_data()
				else
					if have_object then
						result[field] = clone_table(data)
					else
						result[field] = data
					end
				end
			else
				result[field] = data
			end
		end
	end
	return result
end
