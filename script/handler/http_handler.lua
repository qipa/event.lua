local event = require "event"
local util = require "util"



local HANDLER = {}

function HANDLER.log(user,file)
	file = file:gsub("@","/")
	local FILE = io.open(string.format("/data/caiguanqiu/dev/%s/trunk/logic/log/%s",user,file),"r")
	local content = FILE:read("*a")
	FILE:close()
	return content:gsub("\n","<br />"):gsub("\t","&emsp;&emsp;")
end


function HANDLER.index(user)
	local path = string.format("/data/caiguanqiu/dev/%s/trunk/logic/log",user)
	local files = util.list_dir(path,true,nil,true)

	local len = path:len()
	
	local result = {}
	for _,line in ipairs(files) do
		local file = line:sub(len+2)
		table.insert(result,file)
	end

	table.sort(result,function (l,r)
		return l < r
	end)

	local html = {}
	for _,file in pairs(result) do
		table.insert(html,string.format("<a href=\"/log/%s/%s\">%s</a><br />",user,file:gsub("/","@"),file))
	end

	return string.format("<body>%s</body>",table.concat(html,""))
end


function handler(channel,header,url,body)
	channel:session_expire(os.time() + 300)
	local args = url:split("/")

	local method = args[1]
	local user = args[2]

	if HANDLER[method] == nil then
		error("url出错,请检查")
	end

	if not user then
		local cmd = [[
			cd /data/caiguanqiu/dev
			ls -l|awk '{print $9}'
		]]
		local lines = event.run_process(cmd,true)

		local html = {}
		for i = 1,#lines do
			table.insert(html,string.format("<a href=\"/index/%s\">%s</a><br /> <a href=\"/log/%s/engine@runtime.log\">错误日志</a><br />",lines[i],lines[i],lines[i]))
		end
		return string.format("<body>%s</body>",table.concat(html,"<br />"))
	end

	local path = string.format("/data/caiguanqiu/dev/%s",user)
	local attr = lfs.attributes(path)
	if not attr then
		error(string.format("用户:%s不存在",user))
	end
	local result = HANDLER[method](user,table.unpack(args,3))
	return result
end

