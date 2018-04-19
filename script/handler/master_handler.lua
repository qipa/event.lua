local model = require "model"
local server_manager = import "module.server_manager"

_scene_ctx = _scene_ctx or {}
_user_ctx = _user_ctx or {}

function __init__(self)
	server_manager:listen("agent",self,"agent_down")
	server_manager:listen("scene",self,"scene_down")
end

function agent_down(self,server_id)
	for uid,user_info in pairs(_user_ctx) do
		if user_info.agent == server_id then
			server_manager:send_scene(user_info.server,"handler.scene_handler","leave_scene",{scene_uid = user_info.scene_uid,user_uid = user_uid},function (user_uid)
				_user_ctx[user_uid] = nil
			end)
		end
	end
end

function scene_down(self,server_id)
	for uid,user_info in pairs(_user_ctx) do
		if user_info.server == server_id then
			_user_ctx[user_uid] = nil
		end
	end

	for scene_id,scene_info in pairs(_scene_ctx) do
		for scene_uid,info in pairs(scene_info) do
			if info.server == server_id then
				scene_info[scene_uid] = nil
			end
		end
	end
end

function find_scene(scene_id,scene_uid)
	local scene_info = _scene_ctx[scene_id]
	if not scene_info then
		return
	end
	if not scene_uid then
		return
	end
	local info = scene_info[scene_uid]
	if not info then
		return
	end
	return info.server
end

function add_scene(scene_id,scene_uid,server)
	local scene_info = _scene_ctx[scene_id]
	if not scene_info then
		scene_info = {}
		_scene_ctx[scene_id] = scene_info
	end
	scene_info[scene_uid] = {server = server,count = 0}
end

local function do_enter_scene(user_uid,user_agent,scene_server_id,scene_uid,scene_pos,fighter)
	local user_enter_info = {scene_id = scene_id,scene_uid = scene_uid,server = scene_server_id,agent = user_agent}
	_user_ctx[user_uid] = user_enter_info
	server_manager:send_scene(scene_server_id,"handler.scene_channel","enter_scene",{scene_uid = scene_uid,pos = scene_pos,user_uid = user_uid,user_agent = user_agent,fighter = fighter})
end

local function try_enter_scene(user_uid,user_agent,scene_id,scene_uid,scene_pos,fighter)
	local scene_server_id = find_scene(scene_id,scene_uid)
	if not scene_server_id then
		scene_server_id = server_manager:find_min_scene_server()
		server_manager:send_scene(scene_server_id,"handler.scene_handler","create_scene",{scene_id = scene_id},function (scene_uid)
			add_scene(scene_id,scene_uid,scene_server_id)

			do_enter_scene(user_uid,user_agent,scene_server_id,scene_uid,scene_pos,fighter)
		end)
		return
	end
	do_enter_scene(user_uid,user_agent,scene_server_id,scene_uid,scene_pos,fighter)
end

function enter_scene(channel,args)
	local user_uid = args.uid
	local user_agent = args.agent
	local scene_id = args.scene_id
	local scene_uid = args.scene_uid
	local scene_pos = args.scene_pos
	local fighter = args.fighter

	local user_info = _user_ctx[user_uid]
	if not user_info then
		try_enter_scene(user_uid,user_agent,scene_id,scene_uid,scene_pos,fighter)
	else
		server_manager:send_scene(user_info.server,"handler.scene_handler","leave_scene",{scene_uid = scene_uid,user_uid = user_uid,switch = true},function (user_uid)
			_user_ctx[user_uid] = nil
			try_enter_scene(user_uid,user_agent,scene_id,scene_uid,scene_pos,fighter)
		end)
	end
end

function leave_scene(channel,args)
	local user_uid = args.uid
	local user_info = _user_ctx[user_uid]
	server_manager:call_scene(user_info.scene_server_id,"handler.scene_handler","leave_scene",{scene_uid = user_info.scene_uid,user_uid = user_uid,switch = false})
	_user_ctx[user_uid] = nil
	return true
end

