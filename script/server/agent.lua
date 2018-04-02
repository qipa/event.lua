local event = require "event"
local channel = require "channel"
local util = require "util"
local model = require "model"
local protocol = require "protocol"

local rpc = import "server.rpc"

local client_channel = channel:inherit()

model.register_binder("client","id")

local countor = 1

function client_channel:init()
	self.login = false
end

function client_channel:disconnect()

end

function client_channel:forward_login(data,size)
	rpc.send_login("handler.login_handler","client_forward",{id = self.id,data = util.clone_string(data,size)})
end

function client_channel:data(data,size)
	print("data",data,size)
	
	if not self.login then
		self:forward_login(data,size)
	else
		local message = protocol.decode.c2s_auth(data,size)
	end
end


local function client_accept(_,channel)
	channel.id = countor
	countor = countor + 1
	model.bind_client_with_id(channel.id,channel)
end

event.fork(function ()
	protocol.parse("login")
	protocol.load()
	print(event.listen("tcp://0.0.0.0:1989",2,client_accept,client_channel))
end)