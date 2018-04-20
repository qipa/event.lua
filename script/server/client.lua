local event = require "event"
local channel = require "channel"
local util = require "util"
local protocol = require "protocol"


local response = {}

local client_channel = channel:inherit()

function client_channel:disconnect()
	event.error("client_channel disconnect")
end

function client_channel:data(data,size)
	local id,data,size = self.packet:unpack(data,size)
	local name,message = protocol.decode[id](data,size)
	if response[name] then
		response[name](self,message)
	end
end

local agent_channel = channel:inherit()

function agent_channel:disconnect()
	event.error("agent_channel disconnect")
end

function agent_channel:data(data,size)
	local id,data,size = self.packet:unpack(data,size)
	local name,message = protocol.decode[id](data,size)
	if response[name] then
		response[name](self,message)
	end
end

function response.s2c_login_enter(channel,message)
	local channel,err = event.connect(string.format("tcp://%s:%d",message.ip,message.port),2,true,agent_channel)
	if not channel then
		event.error(err)
		return
	end
	channel.packet = util.packet_new()

	channel:write(channel.packet:pack(protocol.encode.c2s_agent_auth({token = message.token})))
end

function response.s2c_create_role(channel,message)
	channel.list = message.list
	local uid = channel.list[1].uid
	channel:write(channel.packet:pack(protocol.encode.c2s_login_enter({account = channel.account,uid = uid})))
end

local _M = {}

function _M.login(channel,account)
	channel.account = account
	channel:write(channel.packet:pack(protocol.encode.c2s_login_auth({account = account})))
	event.sleep(1)
	channel:write(channel.packet:pack(protocol.encode.c2s_create_role({career = 1})))
end


function bench(count)

	for i = 1,count do
		local ip,port = table.unpack(env.login_addr)
		print(string.format("tcp://%s:%d",ip,port))
		local channel,err = event.connect(string.format("tcp://%s:%d",ip,port),2,true,client_channel)
		if not channel then
			event.error(err)
			return
		end
		channel.packet = util.packet_new()
		event.fork(function ()
			_M.login(channel,"mrq"..i)
		end)
		
	end
end

event.fork(function ()
	protocol.parse("login")
	protocol.load()
	
	bench(100)
end)

