local object = import "module.object"

--对应着mongodb中的文档集合(collection)概念
cls_collection = object.cls_base:inherit("database_collection")

function __init__(self)
	self.cls_collection:save_field("__name")
end

function cls_collection:dirty_field(field)
	self.__dirty[field] = true
	self.__dirty["__name"] = true
	self.__parent:dirty_field(self:get_type())
end

function cls_collection:attach_db(parent)
	self.__parent = parent
end

local function clone_table(data,depth)
	depth = depth + 1
	if depth > 3 then
		error(string.format("table is too depth:%d",depth))
	end
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

function cls_collection:save_data(depth)
	if depth ~= 1 then
		error(string.format("cls type:%s inherit from cls_collection,must be first object",self.__name))
	end
	depth = depth + 1
	local save_fields = self.__save_fields
	local set = {}
	local unset = {}
	for field in pairs(self.__dirty) do
		if save_fields[field] ~= nil then
			local data = self[field]
			if data then
				if type(data) == "table" then
					if data.save_data then
						set[field] = data:save_data(depth)
					else
						set[field] = clone_table(data,depth)
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
