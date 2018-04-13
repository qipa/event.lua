local model = require "model"
local server_manager = import "module.server_manager"

_scene_ctx = _scene_ctx or {}
_user_ctx = _user_ctx or {}

function find_scene(scene_id,scene_uid)
	local scene_info = _scene_ctx[scene_id]
	if not scene_info then
		return
	end
	if not scene_uid then
		return
	end
	local scene_server_id = scene_info[scene_uid]
	if not scene_server_id then
		return
	end
	return scene_server_id
end

local function do_enter_scene(user_uid,scene_server_id,scene_uid,scene_pos)
	server_manager:send_scene(scene_server_id,"handler.scene_channel","enter_scene",{scene_uid = scene_uid,pos = scene_pos,user_uid = user_uid})
end

local function try_enter_scene(user_uid,scene_id,scene_uid,scene_pos)
	local scene_server_id = find_scene(scene_id,scene_uid)
	if not scene_server_id then
		scene_server_id = server_manager:find_min_scene_server()
		server_manager:send_scene(scene_server_id,"handler.scene_handler","create_scene",{scene_id = scene_id},function (scene_uid)
			do_enter_scene(user_uid,scene_server_id,scene_uid,scene_pos)
		end)
		return
	end
	do_enter_scene(user_uid,scene_server_id,scene_uid,scene_pos)
end

function enter_scene(channel,args)
	local user_uid = args.uid
	local scene_id = args.scene_id
	local scene_uid = args.scene_uid
	local scene_pos = args.pos

	local user_info = _user_ctx[user_uid]
	if not user_info then
		try_enter_scene(user_uid,scene_id,scene_uid,scene_pos)
	else
		server_manager:send_scene(user_info.scene_server_id,"handler.scene_handler","leave_scene",{scene_uid = scene_uid,user_uid = user_uid},function (user_uid)
			try_enter_scene(user_uid,scene_id,scene_uid,scene_pos)
		end)
	end
end

function leave_scene(channel,args)
	local user_uid = args.uid
	local user_info = _user_ctx[user_uid]
	server_manager:send_scene(user_info.scene_server_id,"handler.scene_handler","leave_scene",{scene_uid = user_info.scene_uid,user_uid = user_uid},function (user_uid)
		_user_ctx[user_uid] = nil
	end)
end

function 