local event = require "event"
local model = require "model"

local server_manager = import "module.server_manager"
local database_manager = import "module.database_manager"
local world_user = import "module.world_user"
import "handler.world_handler"

_agent_server_status = _agent_server_status or {}

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
	import "module.scene_manager"
	local db_channel = model.get_db_channel()
	db_channel:set_db("common")
	local guild_info = db_channel:findAll("guild")
	for _,info in pairs(guild_info) do

	end
end

function stop(self)
	database_common:flush()
	local db_channel = model.get_db_channel()
	local all = model.fetch_world_user()
	for _,user in pairs(all) do
		user:save(db_channel)
	end
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

function server_stop(self,agent_id)
	_agent_server_status[agent_id] = true
	local agent_set = server_manager:how_many_agent()

	local all_agent_done = true
	for _,id in pairs(agent_set) do
		if not _agent_server_status[id] then
			all_agent_done = false
			break
		end
	end

	if all_agent_done then
		local db_channel = model.get_db_channel()
		db_channel:set_db("common")
		
		local updater = {}
		updater["$inc"] = {version = 1}
		updater["$set"] = {time = os.time()}
		db_channel:findAndModify("world_version",{query = {uid = env.dist_id},update = updater,upsert = true})
	end
	return all_agent_done
end