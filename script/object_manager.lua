local event = require "event"

object_mgr = object_mgr or {}

dirty_object = {}

function object_dirty(object)
	dirty_object[object] = true
end

function register(name,...)
	local keys = {...}

	_ENV[string.format("add_%s",name)] = function (obj)
		local object_set = object_mgr[name]
		if object_set == nil then
			object_set = {}
			object_mgr[name] = object_set
		end

		local kv = {}
		for _,k in pairs(keys) do
			local value = obj[k]
			assert(value ~= nil,string.format("object type:%s,havn't unique key:%s",obj:getType(),k))
			kv[k] = value
		end

		for k,v in pairs(kv) do
			local same_key = object_set[k]
			if same_key == nil then
				same_key = {}
				object_set[k] = same_key
			end
			assert(same_key[v] == nil)
			same_key[v] = obj
		end
	end

	_ENV[string.format("remove_%s",name)] = function (obj)
		local object_set = object_mgr[name]
		assert(object_set ~= nil,string.format("no such object type:%s",name))
		for _,k in pairs(keys) do
			local value = obj[k]
			assert(value ~= nil,string.format("object type:%s,havn't unique key:%s",obj:getType(),k))
			local same_key = object_set[k]
			same_key[value] = nil
		end
	end

	_ENV[string.format("all_%s",name)] = function ()
		return object_mgr[name]
	end

	for _,key in pairs(keys) do
		_ENV[string.format("get_%s_by_%s",name,key)] = function (k)
			local object_set = object_mgr[name]
			if object_set == nil then
				return
			end
			local list = object_set[key]
			return list[k]
		end
	end
end
