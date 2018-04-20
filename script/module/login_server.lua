local event = require "event"
local model = require "model"
local route = require "route"
local login_user = import "module.login_user"

_login_ctx = _login_ctx or {}
_account_queue = _account_queue or {}

function start(self)
	self.db_timer = event.timer(30,function ()
		local db_channel = model.get_db_channel()
		local all = model.fetch_login_user()
		for _,user in pairs(all) do
			user:save(db_channel)
		end
	end)
end

function enter(self,cid,addr)
	local info = {cid = cid,addr = addr}
	_login_ctx[cid] = info
	event.error(string.format("cid:%d addr:%s enter",cid,addr))
end

function leave(self,cid)
	event.error(string.format("cid:%d leave",cid))
	local info = _login_ctx[cid]
	if not info then
		return
	end
	
	if info.account then
		local user = model.fetch_login_user_with_account(info.account)
		if user then
			user:leave()
		end
	end
	_login_ctx[cid] = nil
end

function dispatch_client(self,cid,message_id,data,size)
	route.dispatch_client(message_id,data,size,cid)
end

function user_auth(self,cid,account)
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

function user_create_role(self,cid,career)
	local info = _login_ctx[cid]
	if info.account then
		user = model.fetch_login_user_with_account(info.account)
		user:create_role(career)
	end
end

function user_enter_agent(self,cid,uid)
	local info = _login_ctx[cid]
	if info.account then
		local user = model.fetch_login_user_with_account(info.account)
		user:enter_agent(uid)
	end
end

function user_leave_agent(self,account)
	local user = model.fetch_login_user_with_account(account)
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