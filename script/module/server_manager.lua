local model = require "model"

_agent_server_manager = _agent_server_manager or {}
_agent_server_counter = _agent_server_counter or 1

_scene_server_manager = _scene_server_manager or {}
_scene_server_counter = _scene_server_counter or 1

_server_counter = 1

function __init__(self)

end

function apply_id(channel)
	local id = _server_counter
	_server_counter = _server_counter + 1
	return {id = id}
end

function register_agent_server(channel,args)
	local agent = {channel = channel,count = 0,ip = args.ip,port = args.port}
	_agent_server_manager[_agent_server_counter] = agent
	channel.name = "agent"
	channel.id = _agent_server_counter
	_agent_server_counter = _agent_server_counter + 1
	return true
end

function agent_server_down(self,agent_server_id)
	local agent_info = _agent_server_manager[agent_server_id]
	_agent_server_manager[agent_server_id] = nil
end

function agent_count_add(self,agent_server_id)
	local agent = _agent_server_manager[agent_server_id]
	agent.count = agent.count + 1
end

function agent_count_sub(self,agent_server_id)
	local agent = _agent_server_manager[agent_server_id]
	agent.count = agent.count - 1
end

function find_min_agent(self)
	local min
	local best
	local host
	for agent_server_id,agent in pairs(_agent_server_manager) do
		if not min or min < agent.count then
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
	return agent.channel:call(file,method,args,callback)
end

function register_scene_server(channel,addr)
	local scene_server = {channel = channel,count = 0,addr = addr}
	_scene_server_manager[_scene_server_counter] = scene_server
	channel.name = "scene"
	channel.id = _scene_server_counter
	_scene_server_counter = _scene_server_counter + 1
	return true
end

function scene_server_down(self,scene_server_id)
	local scene_server = _scene_server_manager[scene_server_id]
	_scene_server_manager[scene_server_id] = nil
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
		if not min or min < scene_server.count then
			min = scene_server.count
			best = scene_server_id
			addr = scene_server.addr
		end
	end
	return best,addr
end

function send_scene(self,scene_server_id,file,method,args,callback)
	local scene_info = _scene_server_manager[scene_server_id]
	scene_info.channel:send(file,method,args,callback)
end

function call_scene(self,scene_server_id,file,method,args)
	local scene_info = _scene_server_manager[scene_server_id]
	return scene_info.channel:call(file,method,args,callback)
end