local model = require "model"

_agent_manager = _agent_manager or {}

function register_agent(channel,args)
	channel.name = args.name
	local oagent = _agent_manager[args.id]
	if not oagent then
		return false
	end
	local agent = {channel = channel,count = 0,ip = args.ip,port = args.port}
	_agent_manager[args.id] = agent
	return true
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


