local model = require "model"

_agent_manager = _agent_manager or {}
_agent_counter = 1

_scene_server_manager = _scene_server_manager or {}
_scene_server_counter = 1

function register_agent(channel,args)
	local agent = {channel = channel,count = 0,ip = args.ip,port = args.port}
	_agent_manager[_agent_counter] = agent
	channel.name = "agent"
	channel.id = _agent_counter
	_agent_counter = _agent_counter + 1
	return true
end

function agent_down(self,agent_id)
	local agent_info = _agent_manager[agent_id]
	_agent_manager[agent_id] = nil
end

function agent_count_add(self,agent_id)
	local agent = _agent_manager[agent_id]
	agent.count = agent.count + 1
end

function agent_count_sub(self,agent_id)
	local agent = _agent_manager[agent_id]
	agent.count = agent.count - 1
end

function find_min_agent(self)
	local min
	local best
	local host
	for agent_id,agent in pairs(_agent_manager) do
		if not min or min < agent.count then
			min = agent.count
			best = agent_id
			host = {ip = agent.ip,port = agent.port}
		end
	end
	return best,host
end

function send_agent(self,agent_id,file,method,args,callback)
	local agent = _agent_manager[agent_id]
	agent.channel:send(file,method,args,callback)
end

function call_agent(self,agent_id,file,method,args)
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