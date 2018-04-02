local event = require "event"
local channel = require "channel"
local util = require "util"
local protocol = require "protocol"

local console_channel = channel:inherit()

function console_channel:disconnect()
	event.error("console channel disconnect")
end

local function rpc(channel,method,...)
	local result = channel:call_co("handler.master_handler","console",method,...)
	print(table.print(table.decode(result)))
end

local _M = {}

local function pack(data)
	local pat = string.format("I2c%d",data:len())
	return string.pack(pat,data:len()+2,data)
end

function _M.login(channel,account)
	local data = protocol.encode.c2s_auth({account = account})
	print(data,data:len())
	channel:write(pack(data))
end

event.fork(function ()
	protocol.parse("login")
	protocol.load()
	local channel,err = event.connect("tcp://127.0.0.1:1989",2,console_channel)
	if not channel then
		event.error(err)
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
			local func = _M[args[1]]
			if func then
				func(channel,table.unpack(args,2))
			end
		end
	end
end)

