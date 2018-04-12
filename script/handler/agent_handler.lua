local util = require "util"
local protocol = require "protocol"

local agent_user = import "module.agent_user"

_user_token = _user_token or {}
_enter_user = _enter_user or {}

function __init__(self)
	protocol.handler["c2s_enter"] = req_enter
end

function __reload__(self)

end

function req_enter(cid,args)
	local token = args.token
	if not _user_token[token] then
		local client_manager = model.get_client_manager()
		client_manager:close(cid)
		return
	end

	local now = util.time()
	local time = _user_token[token]
	if now - time >= 60 * 100 then
		local client_manager = model.get_client_manager()
		client_manager:close(cid)
		return
	end
	local info = util.authcode(token,time,0)

	local db_channel = model.get_mongodb()
	local user = agent_user.cls_agent_user:new(cid,info.uid)
	user:load(db_channel)
	user:enter_game()
	_enter_user[cid] = user
end

function user_register(args)
	local token = args.token
	local time = args.time
	_user_token[token] = time
end

function user_leave(cid)
	local user = _enter_user[cid]
	if not user then
		return
	end
	user:leave_game()
	user:release()
	_enter_user[cid] = nil
end

function user_kick(args)
	local user = model.fetch_agent_user_with_uid(args.uid)
	local client_manager = model.get_client_manager()
	client_manager:close(cid)
	user:leave_game()
	user:release()
	_enter_user[cid] = nil
end