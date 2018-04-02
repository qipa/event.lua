local event = require "event"
local channel = require "channel"

local CONNECT_STATUS = {
	CLOSED = 1,
	CONNECTING = 2,
	CONNECTED = 3
}

_channel_ctx = {}
_channel_ctx["login"] = {addr = "ipc://login.ipc",status = CONNECT_STATUS.CLOSED,cached_message = {},name = "login"}


local connect_channel = channel:inherit()

function connect_channel:disconnect()
	channel.disconnect(self)
	local ctx = _channel_ctx[self.name]
	ctx.status = CONNECT_STATUS.CLOSED
	event.wakeup(self.monitor)
end

local function do_send(ctx,file,method,args,func)
	if ctx.status == CONNECT_STATUS.CLOSED then
		ctx.status = CONNECT_STATUS.CONNECTING
		event.fork(function ()
			while true do
				local channel,reason = event.connect(ctx.addr,4)
				if not channel then
					event.error(string.format("connect %s failed:%s",ctx.name,reason))
					event.sleep(1)
				else
					event.error(string.format("connect %s success",ctx.name))
					channel.name = ctx.name
					channel.monitor = event.gen_session()
					ctx.status = CONNECT_STATUS.CONNECTED
					ctx.channel = channel
					for _,message in ipairs(ctx.cached_message) do
						print(table.unpack(message))
						ctx.channel:send(table.unpack(message))
					end

					event.wait(channel.monitor)
				end
			end
		end)
	end
	if ctx.status == CONNECT_STATUS.CLOSED or ctx.status == CONNECT_STATUS.CONNECTING then
		table.insert(ctx.cached_message,{file,method,args,func})
		return
	end
	if not func then
		ctx.channel:send(file,method,args)
	else
		ctx.channel:send_back(file,method,args,func)
	end
end

function send_login(file,method,args,func)
	local ctx = _channel_ctx["login"]
	do_send(ctx,file,method,args,func)
end


function call_login(file,method,args)
	local ctx = _channel_ctx["login"]
	assert(ctx.status == CONNECT_STATUS.CONNECTED)
	return ctx.channel:call(file,method,args)
end