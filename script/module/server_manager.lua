local model = require "model"
local event = require "event"
local persistence = require "persistence"
local util = require "util"

_agent_server_manager = _agent_server_manager or {}

_scene_server_manager = _scene_server_manager or {}

_login_server = _login_server or nil

_server_counter = 1

_listener = _listener or {}

function __init__(self)
	self.fs = persistence:open("master")
	local attr = util.attributes("./data/master/dist_id")
	if not attr then
		self._server_counter = 1
		self.fs:save("dist_id",{id = self._server_counter})
	else
		local dist_info = self.fs:load("dist_id")
		self._server_counter = dist_info.id
	end
end

function apply_id(self)
	local id = _server_counter
	_server_counter = _server_counter + 1
	fs:save("dist_id",{id = _server_counter})
	return id
end

function how_many_agent()
	local result = {}
	for id,info in pairs(_agent_server_manager) do
		table.insert(result,id)
	end
	return result
end

function how_many_scene()
	local result = {}
	for id,info in pairs(_scene_server_manager) do
		table.insert(result,id)
	end
	return result
end

function register_agent_server(channel,args)
	local agent = {channel = channel,count = 0,ip = args.ip,port = args.port,id = args.id}
	assert(_agent_server_manager[args.id] == nil,args.id)
	_agent_server_manager[args.id] = agent
	channel.name = "agent"
	channel.id = args.id
	return true
end

function agent_server_down(self,agent_server_id)
	local agent_info = _agent_server_manager[agent_server_id]
	_agent_server_manager[agent_server_id] = nil

	local agent_listener = _listener["agent"]
	if agent_listener then
		for _,info in pairs(agent_listener) do
			info.module[info.method](info.module,agent_server_id)
		end
	end
end

function agent_count_add(self,agent_server_id)
	local agent = _agent_server_manager[agent_server_id]
	agent.count = agent.count + 1
end

function agent_count_sub(self,agent_server_id)
	local agent = _agent_server_manager[agent_server_id]
	agent.count = agent.count - 1
end

function find_agent(self,id)
	return _agent_server_manager[id]
end

function get_agent_channel(self,id)
	local agent_info = _agent_server_manager[id]
	if not agent_info then
		return
	end
	return agent_info.channel
end

function find_min_agent(self)
	local min
	local best
	local host
	for agent_server_id,agent in pairs(_agent_server_manager) do
		if not min or min > agent.count then
			min = agent.count
			best = agent_server_id
			host = {ip = agent.ip,port = agent.port}
		end
	end
	return best,host
end

function send_agent(self,agent_server_id,file,method,args,callback)
	local agent = _agent_server_manager[agent_server_id]
	agent.channel:send(file,method,args,callback)
end

function call_agent(self,agent_server_id,file,method,args)
	local agent = _agent_server_manager[agent_server_id]
	return agent.channel:call(file,method,args)
end

function register_scene_server(channel,args)
	local scene_server = {channel = channel,count = 0,addr = args.addr,id = args.id}
	assert(_scene_server_manager[args.id] == nil,args.id)
	_scene_server_manager[args.id] = scene_server
	channel.name = "scene"
	channel.id = args.id
	return true
end

function scene_server_down(self,scene_server_id)
	local scene_server = _scene_server_manager[scene_server_id]
	_scene_server_manager[scene_server_id] = nil

	local scene_listener = _listener["scene"]
	if scene_listener then
		for _,info in pairs(scene_listener) do
			info.module[info.method](info.module,scene_server_id)
		end
	end
end

function scene_server_add_count(self,scene_server_id)
	local scene_server = _scene_server_manager[scene_server_id]
	scene_server.count = scene_server.count + 1
end

function scene_server_sub_count(self,scene_server_id)
	local scene_server = _scene_server_manager[scene_server_id]
	scene_server.count = scene_server.count - 1
end

function find_min_scene_server(self)
	local min
	local best
	local addr

	for scene_server_id,scene_server in pairs(_scene_server_manager) do
		if not min or min > scene_server.count then
			min = scene_server.count
			best = scene_server_id
			addr = scene_server.addr
		end
	end
	return best,addr
end

function get_scene_addr(self,scene_server_id)
	local scene_info = _scene_server_manager[scene_server_id]
	if not scene_info then
		return
	end
	return scene_info.addr
end

function send_scene(self,scene_server_id,file,method,args,callback)
	local scene_info = _scene_server_manager[scene_server_id]
	if not scene_info then
		event.error(string.format("no such scene server:%d",scene_server_id))
		return
	end
	scene_info.channel:send(file,method,args,callback)
end

function call_scene(self,scene_server_id,file,method,args)
	local scene_info = _scene_server_manager[scene_server_id]
	if not scene_info then
		event.error(string.format("no such scene server:%d",scene_server_id))
		return
	end
	return scene_info.channel:call(file,method,args,callback)
end

function get_scene_channel(self,id)
	local scene_info = _scene_server_manager[id]
	if not scene_info then
		return
	end
	return scene_info.channel
end

function register_login_server(channel,args)
	local login = {channel = channel,id = args.id}
	_login_server = login
	channel.name = "login"
	channel.id = args.id
	return true
end

function login_server_down(self)
	_login_server = nil
end

function listen(self,event,module,method)
	local info = _listener[event]
	if not info then
		info = {}
		_listener[event] = info
	end
	table.insert(info,{module = module,method = method})
end

function broadcast(self,file,method,args)
	local result = {}
	for id,server in pairs(_scene_server_manager) do
		local value = server.channel:call(file,method,args)
		result[id] = value
	end
	for id,server in pairs(_agent_server_manager) do
		local value = server.channel:call(file,method,args)
		result[id] = value
	end
	local value = _login_server.channel:call(file,method,args)
	result[_login_server.id] = value
	return result
end