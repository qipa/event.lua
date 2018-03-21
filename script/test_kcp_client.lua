local event = require "event"
local ikcp = require "ikcp.core"
local util = require "util"


local kcp = ikcp.new(1)
kcp:nodelay(1,10,2,1)
kcp:wndsize(1024,1024)
local udp_session = event.udp(65536,function(udp_session,data,ip,port)
	kcp:input(data)
end,"127.0.0.1",2300)

event.fork(function ()
	event.sleep(0.1)
	for i = 1,1024*1024 do
		kcp:send(string.format("fuck@%d",i))
	end
end)

event.timer(0.01,function (timer)
	local now = math.modf(util.time() * 10)
	kcp:update(now,function (kcp,data)
		udp_session:send("127.0.0.1",2200,data)
	end)
	while true do
		local data = kcp:recv(1024)
		if data then
			print("client recv",data)
		else
			break
		end
	end
end)
