local persistence = require "persistence"
local model = require "model"
require "lfs"

local unique_offset = {
	user = 1,
	item = 2,
	scene = 3,
}

local tmp_offset = {
	monster = 1
}

local dist_offset = 100
local id_section = 100

-- function init(self,dist_id)
-- 	if dist_id >= dist_offset then
-- 		print(string.format("error dist id:%d",dist_id))
-- 		os.exit(1)
-- 	end
	
-- 	for field,offset in pairs(unique_offset) do
-- 		local uname = string.format("%s%d",field,dist_id)
-- 		local attr = lfs.attributes(string.format("./data/id_builder/%s",uname))
-- 		local fs = persistence:open("id_builder")
-- 		local save_info
-- 		if not attr then
-- 			save_info = {begin = 1,offset = id_section}
-- 			fs:save(uname,save_info)
-- 		else
-- 			save_info = fs:load(uname)
-- 			save_info.offset = id_section
-- 			save_info.begin = save_info.begin + save_info.offset
-- 			fs:save(uname,save_info)
-- 		end

-- 		local cursor = save_info.begin
-- 		local max = save_info.begin + save_info.offset

-- 		local cursor = save_info.begin
-- 		local max = save_info.begin + save_info.offset
-- 		self[string.format("alloc_%s_uid",field)] = function ()
-- 			local uid = cursor * dist_offset + dist_id
-- 			cursor = cursor + 1
-- 			if cursor >= max then
-- 				save_info.begin = cursor
-- 				max = save_info.begin + save_info.offset
-- 				fs:save(uname,save_info)
-- 			end
-- 			return uid
-- 		end
-- 	end

-- 	for field,offset in pairs(tmp_offset) do
-- 		local step = 1
-- 		self[string.format("alloc_%s_tid",field)] = function ()
-- 			local tid = step * dist_offset + dist_id
-- 			step = step + 1
-- 			return tid
-- 		end
-- 	end
-- end

function init(self,dist_id)
	if dist_id >= dist_offset then
		print(string.format("error dist id:%d",dist_id))
		os.exit(1)
	end

	local db_channel = model.get_db_channel()
	db_channel:set_db("common")

	for field,offset in pairs(unique_offset) do
		local uname = string.format("%s%d",field,dist_id)
		local result = db_channel:findOne("id_builder",{query = {id = dist_id,key = field}})
		if not result then
			result = {begin = 1,offset = id_section}
		else
			result.begin = result.begin + result.offset
			result.offset = id_section
		end

		local updator = {}
		updator["$set"] = result
		channel:update("id_builder",{id = dist_id,key = field},updator,true)

		local cursor = result.begin
		local max = result.begin + result.offset

		self[string.format("alloc_%s_uid",field)] = function ()
			local uid = cursor * dist_offset + dist_id
			cursor = cursor + 1
			if cursor >= max then
				result.begin = cursor
				max = result.begin + result.offset

				local db_channel = model.get_db_channel()
				db_channel:set_db("common")
				local updator = {}
				updator["$set"] = result
				db_channel:update("id_builder",{id = dist_id,key = field},updator,true)
			end
			return uid
		end
	end

	for field,offset in pairs(tmp_offset) do
		local step = 1
		self[string.format("alloc_%s_tid",field)] = function ()
			local tid = step * dist_offset + dist_id
			step = step + 1
			return tid
		end
	end
end