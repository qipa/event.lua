local event = require "event"
local channel = require "channel"
local util = require "util"
local model = require "model"
local protocol = require "protocol"
local protocol_forward = import "server.protocol_forward"

local rpc = import "server.rpc"

local client_channel = channel:inherit()

model.register_binder("client","id")

local countor = 1

function client_channel:init()
	self.login = false
end

function client_channel:disconnect()

end

function client_channel:forward_login(message_id,data,size)
	rpc.send_login("handler.login_handler","client_forward",{client_id = self.id,message_id = message_id,data = util.clone_string(data,size)},true)
end

function client_channel:data(data,size)
	local id,data,size = self.packet:unpack(data,size)
	if not self.login then
		self:forward_login(id,data,size)
	else
		local id,data,size = self.packet:unpack(data,size)
		local name = protocol_forward.forward[id]
		local message = protocol.decode[name](data,size)
	end
end


local function client_accept(_,channel)
	channel.id = countor
	channel.packet = util.packet_new()
	countor = countor + 1
	model.bind_client_with_id(channel.id,channel)
end

event.fork(function ()
	protocol.parse("login")
	protocol.load()
	print(event.listen("tcp://0.0.0.0:1989",2,client_accept,client_channel))
end)