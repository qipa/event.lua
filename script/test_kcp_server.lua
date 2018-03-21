local event = require "event"
local ikcp = require "ikcp.core"
local util = require "util"

local kcp
local udp_session,err = event.udp(65536,function(udp_session,data,ip,port)
	kcp:input(data)
end,"127.0.0.1",2200)

print(udp_session,err)

kcp = ikcp.new(1)
kcp:nodelay(1,10,2,1)
kcp:wndsize(1024,1024)
local count = 0
event.timer(0.01,function (timer)
	local now = math.modf(util.time() * 10)
	kcp:update(now,function (kcp,data)
		udp_session:send("127.0.0.1",2300,data)
	end)
	while true do
		local data = kcp:recv(1024)
		if data then
			-- kcp:send(string.format("return %s",data))
			-- print("server recv",data)
			count = count + 1
			if count %10000 == 0 then
				print(count)
			end
		else
			break
		end
	end
end)
