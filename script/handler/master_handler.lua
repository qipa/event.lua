local model = require "model"
local server_manager = import "module.server_manager"

_scene_ctx = _scene_ctx or {}
_user_ctx = _user_ctx or {}

local PAHSE = {
	INIT = 1,
	EXECUTE = 2
}

local EVENT = {
	ENTER = 1,
	LEAVE = 2
}

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

function execute_enter_scene(user_info,fighter,scene_id,scene_uid,scene_pos)
	user_info.phase = PHASE.EXECUTE

	if user_info.scene_uid then
		server_manager:call_scene(user_info.scene_server,"handler.scene_handler","leave_scene",{scene_uid = user_info.scene_uid,
																								user_uid = user_info.user_uid,
																								switch = true})
		user_info.scene_id = nil
		user_info.scene_uid = nil
		user_info.scene_server = nil
	end

	local scene_server = find_scene(scene_id,scene_uid)
	if not scene_server then
		scene_server = server_manager:find_min_scene_server()
		scene_uid = server_manager:send_scene(scene_server,"handler.scene_handler","create_scene",{scene_id = scene_id})
		add_scene(scene_id,scene_uid,scene_server)
	end

	server_manager:send_scene(scene_server_id,"handler.scene_channel","enter_scene",{scene_uid = scene_uid,pos = scene_pos,user_uid = user_uid,user_agent = user_agent,fighter = fighter})

	user_info.scene_id = scene_id
	user_info.scene_uid = scene_uid
	user_info.scene_server = scene_server

	user_info.phase = PHASE.INIT

	run_next_event(user_info)
end

function execute_leave_scene(user_info)
	user_info.phase = PHASE.EXECUTE
	server_manager:call_scene(user_info.scene_server_id,"handler.scene_handler","leave_scene",{scene_uid = user_info.scene_uid,
																							   user_uid = user_info.user_uid,
																							   switch = false})


	user_info.phase = PHASE.INIT

	run_next_event(user_info)
end

function run_next_event(user_info)
	local event = table.remove(user_info.event_queue,1)
	if not event then
		return
	end
	if event.ev == EVENT.ENTER then
		execute_enter_scene(user_info,event.fighter,event.scene_id,event.scene_uid,event.scene_pos)
	else
		execute_leave_scene(user_info)
	end
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
		user_info = {user_uid = user_uid,
					 user_agent = user_agent,
					 phase = PHASE.INIT,
					 event_queue = {}}
		_user_ctx[user_uid] = user_info
	end

	table.insert(user_info.event_queue,{ev = EVENT.ENTER
										scene_id = scene_id,
										scene_uid = scene_uid,
										scene_pos = scene_pos,
										fighter = fighter })

	if user_info.phase == PHASE.INIT then
		run_next_event(user_info)
	end
end

function leave_scene(channel,args)
	local user_info = _user_ctx[args.uid]
	if not user_info then
		return true
	end
	table.insert(user_info.event_queue,{ev = EVENT.LEAVE})
	if user_info.phase == PHASE.INIT then
		run_next_event(user_info)
	end
	return true
end

