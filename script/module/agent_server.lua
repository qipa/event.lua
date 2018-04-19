local event = require "event"
local model = require "model"

local agent_user = import "module.agent_user"
local scene_user = import "module.scene_user"
local common = import "common.common"

_user_token = _user_token or {}
_enter_user = _enter_user or {}

local client_manager

function __init__(self)
end

function start(self,client_mgr)
	client_manager = client_mgr
	self.db_timer = event.timer(30,function ()
		local db_channel = model.get_db_channel()
		local all = model.fetch_agent_user()
		for _,user in pairs(all) do
			user:save(db_channel)
		end
	end)
end

function stop(self)
	local db_channel = model.get_db_channel()
	local all = model.fetch_agent_user()
	for _,user in pairs(all) do
		user:save(db_channel)
	end
end

function dispatch_client(self,cid,message_id,data,size)
	local user = model.fetch_agent_user_with_cid(cid)
	if not user then
		route.dispatch_client(message_id,data,size,cid)
	else
		route.dispatch_client(message_id,data,size,user)
	end
end

function enter(self,cid,addr)

end

function leave(self,cid)
	local uid = _enter_user[cid]
	_enter_user[cid] = nil
	if not uid then
		return
	end

	local user = model.fetch_agent_user_with_uid(uid)
	if not user then
		return
	end

	if user.phase == common.AGENT_PHASE.LOAD then
		user.phase = common.AGENT_PHASE.LEAVE
		return
	end

	event.fork(function ()
		local ok,err = xpcall(user.leave_game,debug.traceback,user)
		if not ok then
			event.error(err)
		end

		local world_channel = model.get_world_channel()
		if world_channel then
			world_channel:send("handler.world_handler","leave_world",{user_uid = user.uid})
		end

		local master_channel = model.get_master_channel()
		if master_channel then
			master_channel:send("handler.master_handler","leave_scene",{uid = user.uid})
		end

		local db_channel = model.get_db_channel()
		
		user:save(db_channel)
		user:release()

		local updater = {}
		updater["$inc"] = {version = 1}
		updater["$set"] = {time = os.time()}
		db_channel:findAndModify("save_version",{query = {uid = user.uid},update = updater})

		local login_channel = model.get_login_channel()
		login_channel:send("handler.login_handler","rpc_leave_agent",{account = user.account})
	end)
end

function user_register(self,uid,token,time)
	_user_token[token] = {time = time,uid = uid}
end

function user_kick(self,uid)
	local user = model.fetch_agent_user_with_uid(uid)
	client_manager:close(user.cid)
	leave(user.cid)
end

function user_auth(self,cid,token)
	if not _user_token[token] then
		client_manager:close(cid)
		return
	end

	local info = _user_token[token]
	_user_token[token] = nil

	local now = util.time()
	if now - info.time >= 60 * 100 then
		client_manager:close(cid)
		return
	end

	local info = util.authcode(token,info.time,0)
	if info.uid ~= info.uid then
		client_manager:close(cid)
		return
	end

	local db_channel = model.get_mongodb()
	local user = agent_user.cls_agent_user:new(cid,info.uid)
	
	user.phase = common.AGENT_PHASE.LOAD
	user:load(db_channel)
	
	if user.phase == common.AGENT_PHASE.LEAVE then
		user:release()
		return
	end
	
	user:enter_game()

	local world = model.get_world_channel()
	world:send("handler.world_handler","enter_world",{user_uid = self.uid,user_agent = env.dist_id})

	local fighter = scene_user.cls_scene_user:new(cid,info.uid)
	fighter:load(db_channel)
	local scene_master = model.get_master_channel()
	scene_master:send("handler.master_handler","enter_scene",{uid = user.uid,
															  scene_id = fighter.scene_info.scene_id,
															  scene_uid = fighter.scene_info.scene_uid,
															  scene_pos = fighter.scene_info.scene_pos,
															  agent = env.dist_id,
															  fighter = fighter:pack()})
	fighter:release()


	user.phase = common.AGENT_PHASE.ENTER

	_enter_user[cid] = info.uid
end