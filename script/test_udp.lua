local event = require "event"
local ikcp = require "ikcp.core"
local util = require "util"

local count = 0
local udp_session_server,err = event.udp(65536,function(udp_session,data,ip,port)
	count = count + 1
			if count %10000 == 0 then
				print("recv",count)
			end
end,"127.0.0.1",2200)

local udp_session_client,err = event.udp(65536,function(udp_session,data,ip,port)
	print(data)
end,"127.0.0.1",2300)

event.fork(function ()
	for i = 1,1024*1024 do
		if i %20000 == 0 then
			print("send",i)
			event.sleep(0.1)
		end
		udp_session_client:send("127.0.0.1",2200,"fuck")
	end
end)
