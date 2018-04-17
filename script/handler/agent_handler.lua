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

function enter(cid,addr)

end

function leave(cid)
	local uid = _enter_user[cid]
	_enter_user[cid] = nil
	if not uid then
		return
	end
	local user = model.fetch_agent_user_with_uid(uid)
	if not user then
		return
	end

	event.fork(function ()
		local ok,err = xpcall(user.leave_game,debug.traceback,user)
		if not ok then
			event.error(err)
		end

		local world_channel = model.get_world_channel()
		if world_channel then
			world_channel:call("handler.world_handler","leave_world",{uid = user.uid})
		end

		local master_channel = model.get_master_channel()
		if master_channel then
			master_channel:call("handler.master_handler","leave_scene",{uid = user.uid})
		end

		user:save()
		user:release()

		local db_channel = model.get_db_channel()
		db_channel:db(user:get_type())
		local updater = {}
		updater["$inc"] = {version = 1}
		updater["$set"] = {time = os.time()}
		db_channel:findAndModify("save_version",{query = {uid = user.uid},update = updater})

		local login_channel = model.get_login_channel()
		login_channel:send("handler.login_handler","rpc_leave_agent",{account = user.account})
	end)
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

	_enter_user[cid] = info.uid
end

function user_register(_,args)
	local token = args.token
	local time = args.time
	_user_token[token] = time
end

function user_kick(_,args)
	local user = model.fetch_agent_user_with_uid(args.uid)
	local client_manager = model.get_client_manager()
	client_manager:close(cid)
	leave(user.cid)
end

