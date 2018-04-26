local event = require "event"
local model = require "model"
local util = require "util"
local route = require "route"
local cjson = require "cjson"
local protocol = require "protocol"

local agent_user = import "module.agent_user"
local scene_user = import "module.scene_user"
local common = import "common.common"

_user_token = _user_token or {}
_enter_user = _enter_user or {}
_server_stop = _server_stop or false

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

	self.auth_timer = event.timer(1,function ()
		local login_channel = model.get_login_channel()
		local now = util.time()
		for token,info in pairs(_user_token) do
			if now - info.time >= 60 * 100 then
				_user_token[token] = nil
				login_channel:send("handler.login_handler","rpc_timeout_agent",{account = info.account})
				event.error(string.format("%s:%d auth timeout",info.account,info.uid))
			end
		end
	end)

	import "handler.agent_handler"
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
		local proto = protocol.id_name[message_id]
		local forward = common.PROTOCOL_FORWARD[proto]
		if forward == common.SERVER_TYPE.WORLD then
			user:forward_world(message_id,string.copy(data,size))
		elseif forward == common.SERVER_TYPE.SCENE then
			user:forward_scene(message_id,string.copy(data,size))
		else
			route.dispatch_client(message_id,data,size,user)
		end
	end
end

function enter(self,cid,addr)
end

function leave(self,cid)
	local enter_info = _enter_user[cid]
	_enter_user[cid] = nil
	if not enter_info then
		return
	end

	local user = model.fetch_agent_user_with_uid(enter_info.uid)
	if not user then
		return
	end

	event.fork(function ()
		enter_info.mutex(user_leave,self,user)
	end)
end

function user_register(self,account,uid,token,time)
	_user_token[token] = {time = time,uid = uid,account = account}
end

function user_kick(self,uid)
	local user = model.fetch_agent_user_with_uid(uid)
	if not user then
		for token,info in pairs(_user_token) do
			if info.uid == uid then
				_user_token[token] = nil
				local login_channel = model.get_login_channel()
				login_channel:send("handler.login_handler","rpc_kick_agent",{account = info.account})
				return
			end
		end
		return
	end

	local enter_info = _enter_user[user.cid]
	_enter_user[user.cid] = nil
	if not enter_info then
		return
	end
	client_manager:close(user.cid)
	_enter_user.mutex(user_leave,user)
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

	local token_info = util.authcode(token,info.time,0)
	token_info = cjson.decode(token_info)
	if token_info.uid ~= info.uid then
		client_manager:close(cid)
		return
	end

	local enter_info = {mutex = event.mutex(),uid = info.uid}
	_enter_user[cid] = enter_info

	enter_info.mutex(user_enter,self,cid,info.uid,info.account)
end

function user_enter(self,cid,uid,account)
	local db_channel = model.get_db_channel()
	local user = agent_user.cls_agent_user:new(cid,uid,account)
	
	user:load(db_channel)
	
	user:enter_game()

	local world = model.get_world_channel()
	world:send("handler.world_handler","enter_world",{user_uid = user.uid,user_agent = env.dist_id})

	local fighter = scene_user.cls_scene_user:new(uid)
	fighter:load(db_channel)
	if not fighter.attr then
		fighter.attr = {atk = 100,def = 100}
	end
	if not fighter.scene_info then
		fighter.scene_info = {scene_id = 1001,scene_pos = {x = 100,z = 100}}
	end

	local world_channel = model.get_world_channel()
	world_channel:send("module.scene_manager","enter_scene",{uid = user.uid,
															  scene_id = fighter.scene_info.scene_id,
															  scene_uid = fighter.scene_info.scene_uid,
															  scene_pos = fighter.scene_info.scene_pos,
															  agent = env.dist_id,
															  fighter_data = fighter:pack()})
	fighter:release()
end

function user_leave(self,user)
	local ok,err = xpcall(user.leave_game,debug.traceback,user)
	if not ok then
		event.error(err)
	end

	local world_channel = model.get_world_channel()
	if world_channel then
		world_channel:send("handler.world_handler","leave_world",{user_uid = user.uid})
		world_channel:send("module.scene_manager","leave_scene",{uid = user.uid})
	end

	local db_channel = model.get_db_channel()
	user:save(db_channel)
	
	local updater = {}
	updater["$inc"] = {version = 1}
	updater["$set"] = {time = os.time()}
	db_channel:findAndModify("save_version",{query = {uid = user.uid},update = updater,upsert = true})

	user:release()

	local login_channel = model.get_login_channel()

	login_channel:send("handler.login_handler","rpc_leave_agent",{account = user.account})
end

function get_all_enter_user(self)
	local result = {}
	for cid,info in pairs(_enter_user) do
		local user = model.fetch_agent_user_with_uid(info.uid)
		result[user.account] = {uid = user.uid,agent_server = env.dist_id}
	end
	for _,info in pairs(_user_token) do
		result[info.account] = {uid = info.uid,agent_server = env.dist_id}
	end
	return result
end

function server_stop()
	_server_stop = true
	client_manager:stop()

	for cid,enter_info in pairs(_enter_user)
		client_manager:close(cid)
		local user = model.fetch_agent_user_with_uid(enter_info.uid)
		if user then
			_enter_user.mutex(user_leave,user)
		end
	end

	local db_channel = model.get_db_channel()
	db_channel:set_db("common")
	
	local updater = {}
	updater["$inc"] = {version = 1}
	updater["$set"] = {time = os.time()}
	db_channel:findAndModify("agent_version",{query = {uid = env.dist_id},update = updater,upsert = true})
end