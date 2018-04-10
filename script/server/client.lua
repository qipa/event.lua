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

function _M.login(channel,account)
	local data = protocol.encode.c2s_auth({account = account})
	for i = 1,2 do
		channel:write(channel.packet:pack(1,data))
	end
end

event.fork(function ()
	protocol.parse("login")
	protocol.load()
	local channel,err = event.connect("tcp://127.0.0.1:1989",2,console_channel)
	if not channel then
		event.error(err)
		return
	end
	channel.packet = util.packet_new()

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

