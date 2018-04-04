local util = require "util"
require "lfs" 

local tohex = util.hex_encode
local fromhex = util.hex_decode
local MD5 = util.md5

local _M = {}

function _M:open(db)
	local ctx = setmetatable({},{__index = self})
	local list = db:split("/")

	ctx.db = "./tmp/"..table.concat(list,"/").."/"

	local dir = ""
	local paths = ctx.db:split("/")
	for _,path in ipairs(paths) do
		dir = dir..path.."/"
		local attr = lfs.attributes(dir)
		if not attr then
			lfs.mkdir(dir)
		end
	end

	return ctx
end

function _M:save(k,v)
	local name = self.db..k
	local FILE = assert(io.open(name..".bak","w"))

	local content = table.tostring(v)
	local md5 = MD5(content)
	local hex = tohex(md5)

	FILE:write(hex.."\n")
	FILE:write(content)
	FILE:close()

	os.rename(name..".bak",name)
end

function _M:load(k)
	local FILE = assert(io.open(self.db..k,"r"))
	local omd5 = FILE:read()
	local content = FILE:read("*a")
	local cmd5 = tohex(MD5(content))
	FILE:close()
	if omd5 ~= cmd5 then
		print(string.format("load %s error,try to load bak",k))

		local FILE = assert(io.open(self.db..k..".bak","r"))
		local bak_omd5 = FILE:read()
		local bak_content = FILE:read("*a")
		local bak_cmd5 = tohex(MD5(bak_content))
		FILE:close()
		assert(bak_omd5 == bak_cmd5,string.format("load %s bak error,md5 error",k))
		return table.decode(bak_content)
	end
	return table.decode(content)
end

return _M