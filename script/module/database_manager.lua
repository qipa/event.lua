local event = require "event"
local model = require "model"

_dirty_data = _dirty_data or setmetatable({},{__mode = "k"})
_dirty_object = _dirty_object or setmetatable({},{__mode = "k"})

local FLUSH_TIME = 3 * 60

function __init__(self)
	self.timer = event.timer(FLUSH_TIME,update)
end

function dirty_data(data,name,index)
	_dirty_data[data] = {name = name,index = index}
end

function dirty_object(object,name)
	_dirty_object[object] = {name = name}
end

function update()
	local db_channel = model.get_db_channel()
	db_channel:db("common")

	for data,info in pairs(_dirty_data) do
		local updater = {}
		updater["$set"] = data
		db_channel:update(info.name,info.index,updater,true)
	end
	_dirty_data = setmetatable({},{__mode = "k"})

	for object,info in pairs(_dirty_object) do
		local updater = {}
		if data.save_data then
			updater["$set"] = data:save_data()
		else
			updater["$set"] = data
		end
		
		db_channel:update(info.name,object:db_index(),updater,true)
	end
	_dirty_object = setmetatable({},{__mode = "k"})
end

function flush(self)
	self:update()
end

