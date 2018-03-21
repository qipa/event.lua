local event = require "event"
local http = require "http"


for i = 1,1024  do
event.fork(function ()
	http.post("127.0.0.1:8082","/log/gongcheng/engine@error.log",{},{a = 1,b = 2},function (channel,method,url,header,body)
		-- print(method,url)
		-- table.print(header)
		-- print(body)
	end)
end)
end