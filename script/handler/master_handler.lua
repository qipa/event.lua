local event = require "event"
local model = require "model"

local server_manager = import "module.server_manager"

_scene_ctx = _scene_ctx or {}
_user_ctx = _user_ctx or {}

_enter_mutex = _enter_mutex or {}

local PHASE = {
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
	local set = {}
	for uid,user_info in pairs(_user_ctx) do
		if user_info.agent == server_id then
			table.insert(user_info.event_queue,{ev = EVENT.LEAVE})
			table.insert(set,user_info)
		end
	end

	for _,user_info in pairs(set) do
		if user_info.phase == PHASE.INIT then
			event.fork(function ()
				run_next_event(user_info)
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
		local min_server
		local min_count
		local min_scene_uid
		for scene_uid,info in pairs(scene_info) do
			if not min_count or info.count < min_count then
				min_count = info.count
				min_server = info.server
				min_scene_uid = scene_uid
			end
		end

		if min_server then
			if min_count > 100 then
				return
			end
			return min_server,min_scene_uid
		end
	end

	local info = scene_info[scene_uid]
	if not info then
		return
	end

	if info.count > 100 then
		return
	end
	return info.server,scene_uid
end

function add_scene(scene_id,scene_uid,server)
	local scene_info = _scene_ctx[scene_id]
	if not scene_info then
		scene_info = {}
		_scene_ctx[scene_id] = scene_info
	end
	scene_info[scene_uid] = {server = server,count = 0}
end

function add_scene_count(scene_id,scene_uid)
	local scene_info = _scene_ctx[scene_id]
	local info = scene_info[scene_uid]
	info.count = info.count + 1
	server_manager:scene_server_add_count(info.server)
end

function sub_scene_count(scene_id,scene_uid)
	local scene_info = _scene_ctx[scene_id]
	local info = scene_info[scene_uid]
	info.count = info.count - 1
	server_manager:scene_server_sub_count(info.server)
end

function check_scene(scene_id,scene_uid)
	-- table.print(_scene_ctx,"_scene_ctx")
	local scene_server,scene_uid = find_scene(scene_id,scene_uid)
	if not scene_server then
		scene_server = server_manager:find_min_scene_server()
		scene_uid = server_manager:call_scene(scene_server,"handler.scene_handler","create_scene",{scene_id = scene_id})
		print("create scene",scene_id,scene_server,scene_uid)
		add_scene(scene_id,scene_uid,scene_server)
	end
	return scene_server,scene_uid
end

function execute_enter_scene(user_info,fighter_data,scene_id,scene_uid,scene_pos)

	if user_info.scene_uid then
		server_manager:call_scene(user_info.scene_server,"handler.scene_handler","leave_scene",{scene_uid = user_info.scene_uid,
																								user_uid = user_info.user_uid,
																								switch = true})
		user_info.scene_id = nil
		user_info.scene_uid = nil
		user_info.scene_server = nil
	end

	local mutex = _enter_mutex[scene_id]
	if not mutex then
		mutex = event.mutex()
		_enter_mutex[scene_id] = mutex
	end

	local scene_server,scene_uid = mutex(check_scene,scene_id,scene_uid)

	server_manager:send_scene(scene_server,"handler.scene_handler","enter_scene",{scene_uid = scene_uid,
																				  pos = scene_pos,
																				  user_uid = user_uid,
																				  user_agent = user_agent,
																				  fighter_data = fighter_data})

	user_info.scene_id = scene_id
	user_info.scene_uid = scene_uid
	user_info.scene_server = scene_server

	add_scene_count(scene_id,scene_uid)
end

function execute_leave_scene(user_info)
	server_manager:call_scene(user_info.scene_server,"handler.scene_handler","leave_scene",{user_uid = user_info.user_uid,
																							switch = false})

	sub_scene_count(user_info.scene_id,user_info.scene_uid)
end

function run_next_event(user_info)
	while true do
		local event = table.remove(user_info.event_queue,1)
		if not event then
			return
		end

		if event.ev == EVENT.ENTER then
			user_info.phase = PHASE.EXECUTE
			execute_enter_scene(user_info,event.fighter_data,event.scene_id,event.scene_uid,event.scene_pos)
			user_info.phase = PHASE.INIT
		else
			user_info.phase = PHASE.EXECUTE
			execute_leave_scene(user_info)
			user_info.phase = PHASE.INIT
			if #user_info.event_queue == 0 then
				_user_ctx[user_info.user_uid] = nil
			end
		end
	end
end

function enter_scene(channel,args)

	local user_uid = args.uid
	local user_agent = args.agent
	local scene_id = args.scene_id
	local scene_uid = args.scene_uid
	local scene_pos = args.scene_pos
	local fighter_data = args.fighter_data

	local user_info = _user_ctx[user_uid]
	if not user_info then
		user_info = {user_uid = user_uid,
					 user_agent = user_agent,
					 phase = PHASE.INIT,
					 event_queue = {}}
		_user_ctx[user_uid] = user_info
	end

	table.insert(user_info.event_queue,{ev = EVENT.ENTER,
										scene_id = scene_id,
										scene_uid = scene_uid,
										scene_pos = scene_pos,
										fighter_data = fighter_data })

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

function transfer_scene(_,args)
	local user_info = _user_ctx[args.uid]
	if not user_info then
		return
	end

	table.insert(user_info.event_queue,{ev = EVENT.ENTER,
										scene_id = args.scene_id,
										scene_uid = args.scene_uid,
										scene_pos = args.pos })

	if user_info.phase == PHASE.INIT then
		run_next_event(user_info)
	end
end
