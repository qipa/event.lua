local event = require "event"
local redis = require "redis"
local driver = require "redis.core"

local redis_watcher_channel = redis:inherit()

function redis_watcher_channel:disconnect()
	print("redis_watcher_channel closed")
end

function redis_watcher_channel:init(db)
	self.result = {}
	self.collector = driver.create_collector()
end

function redis_watcher_channel:data()
	self.collector:push(self:read())
	while true do
		local respond = self.collector:pop()
		if respond then
			table.insert(self.result,respond)
			if self.session then
				event.wakeup(self.session)
			end
		else
			break
		end
	end
end

function redis_watcher_channel:auth(password)
	local str = driver.cmd("auth",password)
	self.buffer:write(str)
	return self:wait()
end

function redis_watcher_channel:subscribe(...)
	local str = driver.cmd("subscribe",...)
	self.buffer:write(str)
	return self:wait()
end

function redis_watcher_channel:wait()
	local respond = table.remove(self.result,1)
	if respond == nil then
		self.session = event.gen_session()
		event.wait(self.session)
		self.session = nil
		return table.remove(self.result,1)
	end
	return respond
end



return redis_watcher_channel
