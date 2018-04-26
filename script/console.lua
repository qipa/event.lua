local event = require "event"
local channel = require "channel"
local mongo = require "mongo"
local util = require "util"

local mongo_indexes = import "common.mongo_indexes"

local args = ...

local console_channel = channel:inherit()

function console_channel:disconnect()
	event.error("console channel disconnect")
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
	elseif args == "index" then
		local channel,err = event.connect(env.mongodb,4,true,mongo)
		if not channel then
			event.error(err)
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
		os.exit(0)
	else
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

