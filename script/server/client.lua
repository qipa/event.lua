local event = require "event"
local channel = require "channel"
local util = require "util"
local protocol = require "protocol"

local client_channel = channel:inherit()

function client_channel:disconnect()
	event.error("console channel disconnect")
end

function client_channel:data(data,size)
	local id,data,size = self.packet:unpack(data,size)
	local name,message = protocol.decode[id](data,size)
	table.print(message,name)
end

local _M = {}

function _M.login(channel,account)
	channel:write(channel.packet:pack(protocol.encode.c2s_login_auth({account = account})))
	event.sleep(1)
	channel:write(channel.packet:pack(protocol.encode.c2s_create_role({career = 1})))
	-- channel:write(channel.packet:pack(protocol.encode.c2s_login_enter({account = account,uid = 3001016})))
	
end

event.fork(function ()
	protocol.parse("login")
	protocol.load()
	local ip,port = table.unpack(env.login_addr)
	print(string.format("tcp://%s:%d",ip,port))
	local channel,err = event.connect(string.format("tcp://%s:%d",ip,port),2,true,client_channel)
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

