local event = require "event"
local channel = require "channel"
local util = require "util"

local console_channel = channel:inherit()

function console_channel:disconnect()
	event.error("console channel disconnect")
end

local function rpc(channel,method,...)
	local result = channel:call_co("handler.master_handler","console",method,...)
	print(table.print(table.decode(result)))
end

event.fork(function ()
	local channel,err = event.connect(string.format("tcp://127.0.0.1:%d",env.master),4,console_channel)
	if not channel then
		event.error(err)
		return
	end
	channel:call("handler.master_handler","register","console")
	
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
end)

