local object = import "module.object"

cls_collection = object.cls_base:inherit("database_collection")

function __init__(self)
	self.cls_collection:save_field("__name")
end

function cls_collection:create(parent)
	self.__parent = parent
end

function cls_collection:dirty_field(field)
	self.__dirty[field] = true
	self.__dirty["__name"] = true
	self.__parent:dirty_field(self:get_type())
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

function cls_collection:save_data()
	local set = {}
	local unset = {}
	for field in pairs(self.__dirty) do
		local have_object = self.__save_fields[field]
		if have_object ~= nil then
			local data = self[field]
			if data then
				if type(data) == "table" then
					if data.save_data then
						set[field] = data:save_data()
					else
						if have_object then
							set[field] = clone_table(data)
						else
							set[field] = data
						end
					end
				else
					set[field] = data
				end
			else
				unset[field] = true
			end
		end
	end

	return set,unset
end
