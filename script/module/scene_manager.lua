local event = require "event"
local model = require "model"

local server_manager = import "module.server_manager"
local id_builder = import "module.id_builder"

_scene_ctx = _scene_ctx or {}
_user_ctx = _user_ctx or {}
_agent_server_status = _agent_server_status or {}

function __init__(self)
	server_manager:listen("AGENT_DOWN",self,"agent_down")
	server_manager:listen("SCENE_DOWN",self,"scene_down")
end

function agent_down(self,listener,server_id)
	_agent_server_status[server_id] = nil

	local set = {}
	for uid,user_info in pairs(_user_ctx) do
		if user_info.user_agent == server_id then
			table.insert(set,user_info)
		end
	end

	for _,user_info in pairs(set) do
		event.fork(function ()
			leave_scene(nil,user_info.uid)
		end)
	end
end

function scene_down(self,listener,server_id)
	print("scene_down",server_id)
	for agent_id,agent_info in pairs(_agent_server_status) do
		if agent_info[server_id] then
			agent_info[server_id] = false
		end
	end

	for user_uid,user_info in pairs(_user_ctx) do
		if user_info.scene_server == server_id then
			user_info.scene_id = nil
			user_info.scene_uid = nil
			user_info.scene_server = nil
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

local function find_min_scene(scene_info)
	local min_server
	local min_count
	local min_scene_uid
	for scene_uid,info in pairs(scene_info) do
		if info.clean == false then
			if not min_count or info.count < min_count then
				min_count = info.count
				min_server = info.server
				min_scene_uid = scene_uid
			end
		end
	end

	if not min_server then
		return
	end

	if min_count > 100 then
		return
	end
	return min_server,min_scene_uid
end

local function find_scene(scene_id,scene_uid)
	local scene_info = _scene_ctx[scene_id]
	if not scene_info then
		return
	end
	if not scene_uid or not scene_info[scene_uid] then
		return find_min_scene(scene_info)
	end

	local info = scene_info[scene_uid]
	if info.count > 100 then
		return find_min_scene(scene_info)
	end
	return info.server,scene_uid
end

local function add_scene(scene_id,scene_uid,server)
	local scene_info = _scene_ctx[scene_id]
	if not scene_info then
		scene_info = {}
		_scene_ctx[scene_id] = scene_info
	end
	scene_info[scene_uid] = {server = server,count = 0,clean = false}
end

local function add_scene_count(scene_id,scene_uid)
	local scene_info = _scene_ctx[scene_id]
	local info = scene_info[scene_uid]
	info.count = info.count + 1
	server_manager:scene_server_add_count(info.server)
end

local function sub_scene_count(scene_id,scene_uid)
	local scene_info = _scene_ctx[scene_id]
	local info = scene_info[scene_uid]
	info.count = info.count - 1
	server_manager:scene_server_sub_count(info.server)
end

local function do_enter_scene(scene_id,scene_uid)
	local scene_server,scene_uid = find_scene(scene_id,scene_uid)
	if not scene_server then
		scene_server = server_manager:find_min_scene_server()
		scene_uid = id_builder:alloc_scene_uid()
		server_manager:send_scene(scene_server,"handler.scene_handler","create_scene",{scene_id = scene_id,scene_uid = scene_uid})
		add_scene(scene_id,scene_uid,scene_server)
	end
	add_scene_count(scene_id,scene_uid)

	local scene_addr = server_manager:get_scene_addr(scene_server)
	return scene_server,scene_addr,scene_uid
end

local function do_leave_scene(user_info,switch)
	sub_scene_count(user_info.scene_id,user_info.scene_uid)
	local fighter_data = server_manager:call_scene(user_info.scene_server,"handler.scene_handler","leave_scene",{scene_uid = user_info.scene_uid,
																				  user_uid = user_info.user_uid,
																				  switch = switch})
	local scene_info = _scene_ctx[user_info.scene_id][user_info.scene_uid]
	if scene_info.count == 0 and scene_info.clean then
		_scene_ctx[user_info.scene_id][user_info.scene_uid] = nil
		server_manager:send_scene(scene_server,"handler.scene_handler","delete_scene",{scene_uid = user_info.scene_uid})
	end

	user_info.scene_id = nil
	user_info.scene_uid = nil
	user_info.scene_server = nil
		
	return fighter_data
end

function execute_enter_scene(user_info,fighter_data,scene_id,scene_uid,scene_pos)
	local scene_server,scene_addr,scene_uid = do_enter_scene(scene_id,scene_uid)

	if scene_server == user_info.scene_server then
		server_manager:send_scene(scene_server,"handler.scene_handler","transfer_inside",{user_uid = user_info.user_uid,
																					      scene_uid = scene_uid,
																					      pos = scene_pos})
		user_info.scene_id = scene_id
		user_info.scene_uid = scene_uid
		user_info.scene_server = scene_server
		return
	end

	local agent_info = _agent_server_status[user_info.user_agent]
	local connected = false
	if agent_info and agent_info[scene_server] then
		connected = true
	end

	if not connected then
		local result = server_manager:call_agent(user_info.user_agent,"handler.agent_handler","connect_scene_server",{user_uid = user_info.user_uid, 
																					 scene_server = scene_server,
																					 scene_addr = scene_addr})
		if not result then
			return
		end

		local agent_info = _agent_server_status[user_info.user_agent]
		if not agent_info then
			agent_info = {}
			_agent_server_status[user_info.user_agent] = agent_info
		end
		agent_info[scene_server] = true
	end

	local fighter_data = fighter_data
	if user_info.scene_uid then
		fighter_data = do_leave_scene(user_info,true)
	end

	server_manager:send_scene(scene_server,"handler.scene_handler","enter_scene",{scene_uid = scene_uid,
																				  pos = scene_pos,
																				  user_uid = user_uid,
																				  user_agent = user_info.user_agent,
																				  fighter_data = fighter_data})

	user_info.scene_id = scene_id
	user_info.scene_uid = scene_uid
	user_info.scene_server = scene_server	
end

function execute_leave_scene(user_info)
	do_leave_scene(user_info,false)
end

function enter_scene(_,args)

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
					 mutex = event.mutex()}
		_user_ctx[user_uid] = user_info
	else
		user_info.user_agent = user_agent
	end

	user_info.mutex(execute_enter_scene,user_info,fighter_data,scene_id,scene_uid,scene_pos)
end

function leave_scene(_,args)
	local user_info = _user_ctx[args.uid]
	if not user_info or not user_info.scene_server then
		return true
	end
	user_info.mutex(execute_leave_scene,user_info)
	-- _user_ctx[args.uid] = nil
	return true
end

function transfer_scene(_,args)
	local user_info = _user_ctx[args.uid]
	if not user_info then
		return
	end

	local scene_id = args.scene_id
	local scene_uid = args.scene_uid
	local scene_pos = args.scene_pos

	user_info.mutex(execute_enter_scene,user_info,nil,scene_id,scene_uid,scene_pos)
end

function delete_scene(_,args)
	local scene_info = _scene_ctx[args.scene_id]
	local info = scene_info[args.scene_uid]
	info.clean = true
end

function all_user_leave(self)
	for _,user_info in pairs(_user_ctx) do
		if user_info.scene_uid then
			return false
		end
	end
	return true
end