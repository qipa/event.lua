local event = require "event"
local channel = require "channel"
local mongo = require "mongo"
local util = require "util"
local persistence = require "persistence"

local mongo_indexes = import "common.mongo_indexes"

local args = ...

local console_channel = channel:inherit()

function console_channel:disconnect()
	event.error("console channel disconnect")
end

local function rpc(channel,method,...)
	table.print(channel:call("handler.cmd_handler",method,{...}))
end

event.fork(function ()
	
	if args == "stop" then
		local channel,err = event.connect(env.login,4,true,console_channel)
		if not channel then
			event.error(err)
			os.exit(1)
			return
		end

		local result = channel:call("handler.login_handler","req_stop_server")
		if result then
			os.exit(0)
		else
			os.exit(1)
		end
	elseif args == "startup" then
		local channel,err = event.connect(env.mongodb,4,true,mongo)
		if not channel then
			event.error(string.format("connect mongodb %s failed:%s",env.mongodb,err))
			os.exit(1)
			return
		end

		for db,info in pairs(mongo_indexes) do
			channel:set_db(db)
			for name,indexes in pairs(info) do
				print(string.format("build db:%s,%s index",db,name))
				table.print(channel:ensureIndex(name,indexes))
			end
		end

		local max_uid_ctx = {}
		channel:set_db("login_user")
		local role_list = channel:findAll("role_list",{selector = {list = 1}})
		for _,each_list in pairs(role_list) do
			for _,info in pairs(each_list.list) do
				local dist_id = util.decimal_sub(info.uid,1,2)
				if not max_uid_ctx[dist_id] or info.uid > max_uid_ctx[dist_id] then
					max_uid_ctx[dist_id] = info.uid
				end
			end
		end

		channel:set_db("common")
		local id_builder_info = channel:findAll("id_builder",{query = {key = "user"}})
		local builder_ctx = {}
		for _,info in pairs(id_builder_info) do
			builder_ctx[info.id] = info
		end

		for id,max in pairs(max_uid_ctx) do
			local info = builder_ctx[id]
			if not info then
				print("id builder rebuild error")
				os.exit(1)
			end
			local uid = info.begin * 100 + id
			if uid < max then
				print(string.format("dist id:%d max user uid:%d more than db max uid:%d,need to rebuild",id,max,uid))
				info.begin = math.modf(max / 100) + 1

				local updator = {}
				updator["$set"] = info
				channel:update("id_builder",{id = id,key = "user"},updator,true)
			end
		end

		os.exit(0)
	elseif args == "debugger" then
		local channel,err = event.connect(env.world,4,true,console_channel)
		if not channel then
			event.error(err)
			os.exit(1)
			return
		end
		while true do
			local line = util.readline(">>")
			if line then
				if line == "quit" or line == "q" then
					event.breakout()
					break
				end

				local args = {}
				for i in string.gmatch(line, "%S+") do
					table.insert(args,i)
				end

				local ok,err = pcall(rpc,channel,args[1],table.unpack(args,2))
				if not ok then
					event.error(err)
				end
			end
		end
	end
end)

