local event = require "event"
local util = require "util"
local model = require "model"
local protocol = require "protocol"
local mongo = require "mongo"
local channel = require "channel"
local route = require "route"
local http = require "http"
local startup = import "server.startup"
local id_builder = import "module.id_builder"
local server_manager = import "module.server_manager"
local login_server = import "module.login_server"
local mongo_indexes = import "common.mongo_indexes"
model.register_value("client_manager")
model.register_binder("agent_channel","id")


local common_channel = channel:inherit()
function common_channel:disconnect()
	if self.id ~= nil then
		if self.name == "agent" then
			server_manager:agent_server_down(self.id)
		end
	end
end


local function channel_accept(_,channel)
	
end

local function client_data(cid,message_id,data,size)
	local ok,err = xpcall(login_server.dispatch_client,debug.traceback,login_server,cid,message_id,data,size)
	if not ok then
		event.error(err)
	end
end

local function client_accept(cid,addr)
	local ok,err = xpcall(login_server.enter,debug.traceback,login_server,cid,addr)
	if not ok then
		event.error(err)
	end
end

local function client_close(cid)
	local ok,err = xpcall(login_server.leave,debug.traceback,login_server,cid)
	if not ok then
		event.error(err)
	end
end

event.fork(function ()
	startup.run(env.mongodb)

	env.dist_id = startup.apply_id()
	id_builder:init(env.dist_id)
	
	local ok,reason = event.listen(env.login,4,channel_accept,common_channel)
	if not ok then
		event.breakout(reason)
		return
	end

	local agent_set
	while true do
		local result,reason  = http.post_master("/howmany_agent")
		if not result then
			event.error(string.format("howmany_agent error:%s",reason))
			event.breakout(reason)
			return
		end
		if #result == 0 then
			event.sleep(1)
		else
			agent_set = result
			break
		end
	end

	local agent_ready = true
	while true do
		for _,agent_id in pairs(agent_set) do
			if not server_manager:find_agent(agent_id) then
				agent_ready = false
				break
			end
		end
		if agent_ready then
			break
		else
			event.sleep(1)
		end
	end

	event.error(string.format("login:all agent ready:%s",table.concat(agent_set,"\t")))
	
	local client_manager = event.client_manager(5000)
	client_manager:set_callback(client_accept,client_close,client_data)
	local ok,reason = client_manager:start(table.unpack(env.login_addr))
	if not ok then
		event.breakout(string.format("login listen client failed:%s",reason))
		return
	end
	event.error(string.format("login listen client success",reason))
	model.set_client_manager(client_manager)

	local db_channel = model.get_db_channel()
	db_channel:set_db("login_user")
	for name,indexes in pairs(mongo_indexes.login_user) do
		db_channel:ensureIndex(name,indexes)
	end


	import "handler.login_handler"
end)