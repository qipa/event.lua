local event = require "event"
local channel = require "channel"
local util = require "util"
local helper = require "helper"

local client_channel = channel:inherit()

function client_channel:disconnect()
	channel.disconnect(self)
	if self.monitor_session then
		event.wakeup(self.monitor_session)
	end
end


event.fork(function ()
	local channel_session,err
	while not channel_session do
		channel_session,err = event.connect("ipc://rpc.ipc",4,client_channel)
		if channel_session then
			
			local now = util.time()
			for i = 1,1024 * 1024 do
				channel_session:send("handler.test_handler","test_rpc",{hello = "world"})
			end
			print(channel_session:call("handler.test_handler","test_rpc",{hello = "world"}))
			print("done",util.time() - now)
			-- event:breakout()
			channel_session.monitor_session = event.gen_session()
			event.wait(channel_session.monitor_session)
			channel_session = nil
		else
			event.sleep(1)
			print(channel_session,err)
		end
	end
end)

local count = 0
event.timer(1,function (timer)

		collectgarbage("collect")
		local lua_mem = collectgarbage("count")
		helper.free()
		print(string.format("lua mem:%fkb,c mem:%fkb",lua_mem,helper.allocated()/1024))
	
	count = count + 1
end)
