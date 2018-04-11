local util = require "util"
local helper = require "helper"

-- local code = util.authcode("mrq","fuck",1)
-- print(util.authcode(code.."1","fuck",0))
local str = "mrq"
	local count = 1024
	for j = 1,count do
		str= str.."f"
	end
local now = util.time()
for i = 1,1024 * 100 do
	
	util.authcode(str,"fuck",1)
end

local lua_mem = collectgarbage("count")
print(string.format("lua mem:%fkb,c mem:%fkb",lua_mem,helper.allocated()/1024))
print("time diff:",(util.time() - now) * 10)