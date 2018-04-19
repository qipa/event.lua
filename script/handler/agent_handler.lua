local util = require "util"
local protocol = require "protocol"

local agent_server = import "module.agent_server"

function __init__(self)
	protocol.handler["c2s_agent_auth"] = req_auth
end

function req_auth(cid,args)
	agent_server:user_auth(cid,args.token)
end

function user_register(_,args)
	agent_server:user_register(args.uid,args.token,args.time)
end

function user_kick(_,args)
	agent_server:user_kick(args.uid)
end

function forward_client(_,args)
	local user = model.fetch_agent_user_with_uid(args.uid)
	if not user then
		return
	end
	user:forward_client(args.message_id,args.data)
end