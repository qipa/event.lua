local protocol = require "protocol"
local cjson = require "cjson"
local util = require "util"
local model = require "model"


_login_ctx = _login_ctx or {}
_account_queue = _account_queue or {}

function enter(cid,addr)
	local info = {cid = cid,addr = addr}
	_login_ctx[cid] = info
end

function leave(cid)
	local info = _login_ctx[cid]
	if info.account then
		local login_user = model.fetch_login_user_with_account(account)
		if login_user then
			login_user:leave()
		end
	end
	_login_ctx[cid] = nil
end

function auth(cid,args)
	local account = args.account
	
	local queue = _account_queue[account]
	if not queue then
		queue = {}
		_account_queue[account] = queue
	end
	table.insert(queue,cid)

	local login_user = model.fetch_login_user_with_account(account)
	if login_user then
		login_user:leave(function ()
			local last_cid = table.remove(queue)
			local client_manager = model.get_client_manager()
			for _,cid in pairs(queue) do
				client_manager:close(cid)
				leave(cid)
			end

			_account_queue[account] = nil
		end)
	else
		assert(#queue == 1)
		_account_queue[account] = nil

		local info = _login_ctx[cid]
		info.account = account
		login_user:new(cid,account)
	end
end

function enter_game(client_id,args)
	local account = args.account
	local user_id = args.user_id
	local json = cjson.encode({account = account,user_id = user_id})
	local md5 = util.md5(json)
	local now = util.time()
	local token = util.rc4(json,now)

end