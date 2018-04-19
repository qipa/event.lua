local event = require "event"
local model = require "model"

local database_manager = import "module.database_manager"
local world_user = import "module.world_user"

function __init__(self)
	self.timer = event.timer(30,function ()
		local db_channel = model.get_db_channel()
		local all = model.fetch_world_user()
		for _,user in pairs(all) do
			user:save(db_channel)
		end
	end)
end

function start(self)
	local db_channel = model.get_db_channel()
	db_channel:set_db("common")
	local guild_info = db_channel:findAll("guild")
	for _,info in pairs(guild_info) do

	end
end

function stop(self)
	database_common:flush()
end

function enter(self,user_uid,user_agent)
	local user = model.fetch_world_user_with_uid(user_uid)
	if user then
		user:leave()
		user:release()
	end

	local db_channel = model.get_db_channel()
	
	local user = world_user.cls_world_user:new(user_uid,user_agent)
	user.loading = true
	user:load(db_channel)
	user.loading = false

	local user = model.fetch_world_user_with_uid(user_uid)
	if not user then
		return
	end

	user:enter()
end

function leave(self,user_uid)
	local user = model.fetch_world_user_with_uid(user_uid)
	if not user then
		return false
	end
	
	if user.loading then
		user:release()
		return
	end

	user:leave()
	
	local db_channel = model.get_db_channel()
	user:save(db_channel)
	user:release()
	return true
end
