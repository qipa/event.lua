
local model = require "model"
local event = require "event"

MODEL_BINDER("channel_ctx","channel")

local _M = {}


local channel_ctx = {}

function enter(channel)
	local ctx = {channel = channel}
	model.bind_channel_ctx_with_channel(channel,ctx)
end

function leave(channel)
	model.unbind_channel_ctx_with_channel(channel)
end

function register(name,port)
	local ctx = model.fetch_channel_ctx_with_channel(channel)
	ctx.name = name
	ctx.port = port
	return "ok"
end

function get_info(name)
	local result = {}
	for ch,ctx in pairs(model.fetch_channel_ctx()) do
		if ctx.name == name then
			table.insert(result,{port = ctx.port})
		end
	end
	return result
end

function console(cmd,...)
	local args = {...}
	local session = event.gen_session()
	event.fork(function ()
		local list = {}
		
		if cmd == "stop" then
			for ch,ctx in pairs(model.fetch_channel_ctx_with_channel()) do
				ch:send("handler.cmd_handler",cmd)
			end

			while true do
				local channel_ctx = model.fetch_channel_ctx_with_channel()
				if next(channel_ctx) == nil then
					break
				end
				event.sleep(1)
			end
			event.breakout()
		else
			for ch,ctx in pairs(model.fetch_channel_ctx_with_channel()) do
				local ok,result = pcall(ch.call,ch,"handler.cmd_handler",cmd,table.unpack(args))
				table.insert(list,{name = ctx.name,result = result})
			end
			event.wakeup(session,table.tostring(list))
		end
	end)
	return event.wait(session)
end

