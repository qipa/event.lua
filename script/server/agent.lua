local event = require "event"
local channel = require "channel"
local util = require "util"
local model = require "model"

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
	rpc.send_login("handler.login_handler","client_forward",{id = self.id,data = util.tostring(data,size)})
end

function client_channel:data(data,size)
	if not self.login then
		self:forward_login(data,size)
	else
		local message = table.decode(data,size)
	end
end


local function client_accept(_,channel)
	channel.id = countor
	countor = countor + 1
	model.bind_client_with_id(channel.id,channel)
end

event.fork(function ()
	event.listen("tcp://0.0.0.0:1989",4,client_accept)
end)