local event = require "event"
local util = require "util"
local model = require "model"

local pairs = pairs
local setmetatable = setmetatable
local type = type

class_ctx = class_ctx or {}

object_ctx = object_ctx or {}

cls_base = cls_base or { __name = "root", __childs = {}}

class_ctx[cls_base.__name] = cls_base

local function clean_cls_func(cls)
	for name in pairs(cls.__childs) do
		local child_cls = class_ctx[name]
		for method in pairs(child_cls.__method) do
			child_cls[method] = nil
		end
		child_cls.__method = {}
		
		clean_cls_func(child_cls)
	end
end

local function reset_object_meta(name)
	local cls = class_ctx[name]
	local object_set = object_ctx[name]
	if not object_set then
		return
	end
	for obj in pairs(object_set) do
		setmetatable(obj,{__index = cls})
	end
end

function cls_base:inherit(name,...)
	local parent = class_ctx[self.__name]

	local old_cls = class_ctx[name]

	local cls = {}
	cls.__name = name
	cls.__parent = parent.__name
	cls.__childs = {}
	cls.__save_fields = {}
	cls.__pack_fields = {}
	if parent.__save_fields then
		for f in pairs(parent.__save_fields) do
			cls.__save_fields[f] = true
		end
	end
	if parent.__pack_fields then
		for f in pairs(parent.__pack_fields) do
			cls.__pack_fields[f] = true
		end
	end
	cls.__method = {}
	cls.__pack_fields["__name"] = true

	assert(name ~= parent.__name)

	local meta = { __index = function (obj,method)
		local func = parent[method]
		cls.__method[method] = true
		cls[method] = func
		return func
	end}

	local cls = setmetatable(cls,meta)
	
	class_ctx[name] = cls

	if old_cls ~= nil then
		--热更
		clean_cls_func(old_cls)

		for name in pairs(old_cls.__childs) do
			local child_cls = class_ctx[name]
			local child_cls = setmetatable(child_cls,{ __index = function (obj,method)
				local func = cls[method]
				child_cls.__method[method] = true
				child_cls[method] = func
				return func
			end})
		end

		reset_object_meta(name)
	else
		if select("#",...) > 0 then
			model.register_binder(name,...)
		end
	end
	parent.__childs[name] = true

	return cls
end

function cls_base:get_type()
	return self.__name
end

local function new_object(self,obj)
	setmetatable(obj,{__index = self})

	local object_type = self:get_type()
	local object_set = object_ctx[object_type]
	if object_set == nil then
		object_set = setmetatable({},{__mode = "k"})
		object_ctx[object_type] = object_set
	end

	object_set[obj] = {time = os.time()}
end


function cls_base:new(...)
	local object = { __dirty = {},
				 	 __name = self.__name,
				 	 __alive = true,
				 	 __event = {}}
	local self = class_ctx[self.__name]
	new_object(self,object)

	object:create(...)

	return object
end

function cls_base:instance_from(data)
	local object_type = self:get_type()
	local class = class_ctx[object_type]
	local object = { __dirty = {},
				 	 __name = self.__name,
				 	 __alive = true,
				 	 __event = {}}
	new_object(class,object)
	for k,v in pairs(data) do
		object[k] = v
	end
	return object
end

function cls_base:release()
	self.__alive = false
	self:destroy()
end

--子类重写
function cls_base:create(...)

end

--子类重写
function cls_base:init()

end

--子类重写
function cls_base:destroy()

end

local function clone_table(data)
	local result = {}
	for k,v in pairs(data) do
		if type(v) == "table" then
			if v.__name then
				result[k] = v:pack(true)
			else
				result[k] = clone_table(v)
			end
		else
			result[k] = v
		end
	end
	return result
end

function cls_base:pack()
	local cls = class.get(self:get_type())
	local pack_fields = cls.__pack_fields
	local save_fields = cls.__save_fields
	local result = {}
	for k,v in pairs(self) do
		if pack_fields[k] or save_fields[k] then
			result[k] = v
		end
	end
	return result
end

function cls_base:unpack(...)
	local inst = table.decode(...)
	return class.instance_from(self.__name,inst)
end

function cls_base:save_field(field)
	self.__save_fields[field] = true
end

function cls_base:pack_field(field)
	self.__pack_fields[field] = true
end

function cls_base:register_event(inst,ev,method)
	local ev_list = self.__event[ev]
	if not ev_list then
		ev_list = {}
		self.__event[ev] = ev_list
	end
	ev_list[inst] = method
end

function cls_base:deregister_event(inst,ev)
	local ev_list = self.__event[ev]
	if not ev_list then
		return
	end
	ev_list[inst] = nil
end

function cls_base:fire_event(ev,...)
	local ev_list = self.__event[ev]
	if not ev_list then
		return
	end
	for inst,method in pairs(ev_list) do
		local func = inst[method]
		if not func then
			event.error(string.format("fire event error,no such method:%s",method))
		else
			local ok,err = xpcall(func,debug.traceback,inst,self,...)
			if not ok then
				event.error(err)
			end
		end
	end
end

class = {}

function class.new(name,...)
	local cls = class.get(name)
	return cls:new(...)
end

local function foreach_instance(inst)
	for k,v in pairs(inst) do
		if type(v) == "table" then
			if v.__name then
				inst[k] = class.instance_from(v.__name,v)
			else
				foreach_instance(v)
			end
		end
	end
end

function class.instance_from(name,data)
	local cls = class.get(name)
	assert(cls ~= nil,name)
	local inst = cls:instance_from(data)
	foreach_instance(inst)
	return inst
end

function class.get(name)
	return class_ctx[name]
end

function class.detect_leak()
	collectgarbage("collect")

	local leak_obj = {}

	for name in pairs(class_ctx) do
		local object_set = object_ctx[name]
		if object_set then
			for weak_obj,info in pairs(object_set) do
				local alive_object_ctx = model[string.format("fetch_%s",name)]()
				local alive = false

				if alive_object_ctx then
					local _,alive_object = next(alive_object_ctx)
					for _,obj in pairs(alive_object) do
						if weak_obj == obj then
							alive = true
							break
						end
					end
				end
				if not alive then
					leak_obj[weak_obj] = info
				end
			end
		end
	end
	
	local log = {}
	for obj,info in pairs(leak_obj) do
		table.insert(log,string.format("object type:%s,create time:%s,debug info:%s",obj:get_type(),os.date("%m-%d %H:%M:%S",math.floor(info.time/100)),obj.__debugInfo or "unknown"))
	end
	print("-----------------detect leak object-----------------")
	print(table.concat(log,"\n"))
end

_G["class"] = class
rawset(_G,"class",class)