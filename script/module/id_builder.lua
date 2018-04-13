local persistence = require "persistence"
require "lfs"

local save_offset = {
	user = 1,
	item = 2
}

local tmp_offset = {
	scene = 1,
	monster = 2
}


function init(self,dist_id)
	for field,offset in pairs(save_offset) do
		local uname = string.format("%s%d",field,dist_id)
		local attr = lfs.attributes(string.format("./tmp/id_builder/%s",uname))
		local fs = persistence:open("id_builder")
		local save_info
		if not attr then
			save_info = {begin = 10000 + dist_id,offset = 100}
			fs:save(uname,save_info)
		else
			save_info = fs:load(uname)
			save_info.begin = save_info.begin + save_info.offset
			fs:save(uname,save_info)
		end

		local cursor = save_info.begin
		local max = save_info.begin + save_info.offset

		local cursor = save_info.begin
		local max = save_info.begin + save_info.offset
		self[string.format("alloc_%s_uid",field)] = function ()
			local uid = cursor
			cursor = cursor + 1
			if cursor >= max then
				save_info.begin = cursor
				max = save_info.begin + save_info.offset
				fs:save(uname,save_info)
			end
			return uid
		end
	end

	for field,offset in pairs(tmp_offset) do
		local step = 1
		self[string.format("alloc_%s_tid",field)] = function ()
			local tid = step * 10000 + dist_id
			step = step + 1
			return tid
		end
	end
end