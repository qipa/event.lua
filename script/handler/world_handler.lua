local model = require "model"
local world_user = import "module.world_user"


function enter_world(channel,args)
	local user = model.fetch_world_user_with_uid(args.uid)
	if user then
		user:leave()
		user:release()
	end

	local db_channel = model.get_db_channel()
	local agent_server_id = channel.id
	local user = world_user.cls_world_user:new(args.uid,args.cid,agent_server_id)
	user:load(db_channel)
	user:enter()
end

function leave_world(channel,args)
	local user = model.fetch_world_user_with_uid(args.uid)
	if not user then
		return false
	end
	user:leave()
	local db_channel = model.get_db_channel()
	user:save(db_channel)
	user:release()
	return true
end