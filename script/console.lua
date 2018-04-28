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

		local max_user_uid
		for db,info in pairs(mongo_indexes) do
			channel:set_db(db)
			for name,indexes in pairs(info) do
				print(string.format("build db:%s,%s index",db,name))
				table.print(channel:ensureIndex(name,indexes))
			end
			local base_info = channel:findAll("base_info",{selector = {uid = 1}})
			if #base_info ~= 0 then
				table.sort(base_info,function (l,r)
					return l.uid > r.uid
				end)
				if not max_user_uid or base_info[1].uid > max_user_uid then
					max_user_uid = base_info[1].uid
				end
			end
		end

		local fs = persistence:open("id_builder")
		local list = util.list_dir("./data/id_builder")
		for _,name in pairs(list) do
			if name:match("user") then
				local data = fs:load(name)
			end
		end
		-- channel:set_db("common")
		-- local query = {}
		-- query["uid"] = {["$gt"] = 100000000}
		-- channel:findAll("scene_info",{query = query})

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

