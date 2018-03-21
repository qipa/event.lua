local util = require "util"


local now = util.time()
for i = 1,1024*10 do
	util.getaddrinfo("www.baidu.com",0)
end
print("done",util.time() - now)