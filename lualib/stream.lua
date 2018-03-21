
local channel = require "channel"

local stream_channel = channel:inherit()

function stream_channel:disconnect()
	local event = require "event"
	event.wakeup(self.session)
end

function stream_channel:data()
	local data = self:read()
	if self.stream then
		self.stream = self.stream..data
	else
		self.stream = data
	end
end

function stream_channel:wait()
	local event = require "event"
	self.session = event.gen_session()
	event.wait(self.session)
	return self.stream
end

function stream_channel:wait_lines()
	local event = require "event"
	self.session = event.gen_session()
	event.wait(self.session)
	local result = {}
	if self.stream then
		for line in string.gmatch(self.stream,".[^\r\n]+") do
			line = line:gsub("[\r\n]","")
			table.insert(result,line)
		end
	end
	return result
end

return stream_channel
