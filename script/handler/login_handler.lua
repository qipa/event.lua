local protocol = require "protocol"
local cjson = require "cjson"
local util = require "util"
local model = require "model"


_login_ctx = _login_ctx or {}

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

function kick_client(cid)
	local client_manager = model.get_client_manager()
	client_manager:close(cid)
end

function auth(cid,args)
	local account = args.account
	local login_user = model.fetch_login_user_with_account(account)
	if login_user then
		kick_client(login_user.cid)
		login_user:leave()
	end
	local info = _login_ctx[cid]
	info.account = account
	login_user:new(cid,account)
end

function enter_game(client_id,args)
	local account = args.account
	local user_id = args.user_id
	local json = cjson.encode({account = account,user_id = user_id})
	local md5 = util.md5(json)
	local now = util.time()
	local token = util.rc4(json,now)

end