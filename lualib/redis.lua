local event = require "event"
local channel = require "channel"
local driver = require "redis.core"

local redis_channel = channel:inherit()

function redis_channel:disconnect()
	print("redis_channel closed")
end

function redis_channel:init(db)
	self.request_ctx = {}
	self.request_id = 1
	self.respond_id = 1
	self.collector = driver.create_collector()
end

function redis_channel:data()
	self.collector:push(self:read())
	while true do
		local respond = self.collector:pop()
		if respond then
			local respond_id = self.respond_id
			self.respond_id = self.respond_id + 1
			local respond_info = self.request_ctx[respond_id]
			if not respond_info.session then
				local ok,err = xpcall(respond_info.func,debug.traceback,respond)
				if not ok then
					print(err)
				end
				self.request_ctx[self.request_id] = nil 
			else
				event.wakeup(respond_info.session,respond)
			end
		else
			break
		end
	end
end

function redis_channel:cmd(...)
	local str = driver.cmd(...)
	self.buffer:write(str)
	local session = event.gen_session()
	self.request_ctx[self.request_id] = {session = session}
	self.request_id = self.request_id + 1
	local result = event.wait(session)
	self.request_ctx[self.request_id] = nil
	return result
end

function redis_channel:cmd_callback(func,...)
	local str = driver.cmd(...)
	self.buffer:write(str)
	self.request_ctx[self.request_id] = {func = func}
	self.request_id = self.request_id + 1
end



return redis_channel
