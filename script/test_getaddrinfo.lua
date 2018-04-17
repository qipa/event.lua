local util = require "util"


util.time_diff("getaddrinfo",function ()
	for i = 1,1024*10 do
		util.getaddrinfo("www.baidu.com",0)
	end
end)
