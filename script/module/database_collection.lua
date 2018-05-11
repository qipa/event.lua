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

function cls_collection:save_data()
	local save_fields = self.__save_fields
	local set
	local unset
	for field in pairs(self.__dirty) do
		if save_fields[field] ~= nil then
			local data = self[field]
			if data then
				if not set then
					set = {}
				end
				set[field] = data
			else
				if not unset then
					unset = {}
				end
				unset[field] = true
			end
		end
	end

	return set,unset
end
