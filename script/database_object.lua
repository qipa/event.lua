local object = import "object"

cls_database = object.cls_base:inherit("database_object")

function cls_database:dirty_field(field)
	self.__dirty[field] = true
end

--子类重写,返回保存数据库的索引
function cls_database:db_index()
	return {id = self.__uid}
end

function cls_database:load(db_channel)
	for field in pairs(self.__save_fields) do
		local result = db_channel:findOne(field,{query = self:db_index()})
		if result then
			local cls = class.get(field)
			if cls then
				local field_obj = class.instance_from(field,result)
				self[field] = field_obj
			else
				self[field] = result
			end
		end
	end
end

function cls_database:save(db_channel)
	for field in pairs(self.__dirty) do
		if self.__save_fields[field] then
			local data = self[field]
			if data then
				local updater = {}
				if data.save_data then
					updater["$set"] = data:save_data()
				else
					updater["$set"] = data
				end
				
				db_channel:update(field,self:db_index(),updater,true)
			end
		end
	end
	self.__dirty = {}
end
