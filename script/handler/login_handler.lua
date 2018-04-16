local event = require "event"
local protocol = require "protocol"
local cjson = require "cjson"
local util = require "util"
local model = require "model"

local login_user = import "module.login_user"

_login_ctx = _login_ctx or {}
_account_queue = _account_queue or {}

function __init__()
	protocol.handler["c2s_login_auth"] = req_enter
	protocol.handler["c2s_login_enter"] = req_enter_game
	protocol.handler["c2s_create_role"] = req_create_role
	
end

function enter(cid,addr)
	local info = {cid = cid,addr = addr}
	_login_ctx[cid] = info
	event.error(string.format("cid:%d addr:%s enter",cid,addr))
end

function leave(cid)
	event.error(string.format("cid:%d leave",cid))
	local info = _login_ctx[cid]
	if not info then
		return
	end
	
	if info.account then
		local user = model.fetch_login_user_with_account(account)
		if user then
			user:leave()
		end
	end
	_login_ctx[cid] = nil
end

function req_enter(cid,args)
	local account = args.account
	
	local queue = _account_queue[account]
	if not queue then
		queue = {}
		_account_queue[account] = queue
	end
	table.insert(queue,cid)

	local user = model.fetch_login_user_with_account(account)
	if user then
		if user:leave() then
			assert(#queue == 1)
			_account_queue[account] = nil
			local info = _login_ctx[cid]
			if info then
				info.account = account
				local user = login_user.cls_login_user:new(cid,account)
				user:auth()
			end
		else
			user:kick_agent()
		end
	else
		assert(#queue == 1)
		_account_queue[account] = nil

		local info = _login_ctx[cid]
		if info then
			info.account = account
			local user = login_user.cls_login_user:new(cid,account)
			user:auth()
		end
	end
end

function req_create_role(cid,args)
	local info = _login_ctx[cid]
	if info.account then
		user = model.fetch_login_user_with_account(info.account)
		user:create_role(args.career)
	end
end

function req_enter_game(client_id,args)
	local account = args.account
	local user_id = args.user_id
	local user = model.fetch_login_user_with_account(account)
	user:enter_agent(user_id)
end

function rpc_leave_agent(self,args)
	local user = model.fetch_login_user_with_account(args.account)
	if not user then
		return
	end
	user:leave_agent()

	local queue = _account_queue[user.account]
	if not queue then
		return
	end

	local client_manager = model.get_client_manager()
	for _,cid in pairs(queue) do
		if last_cid ~= cid then
			client_manager:close(cid)
			_login_ctx[cid] = nil
		end
	end
	_account_queue[account] = nil
	local info = _login_ctx[last_cid]
	if info then
		info.account = account
		local user = login_user.cls_login_user:new(last_cid,account)
		user:auth()
	end

end