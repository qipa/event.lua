local event = require "event"
local util = require "util"
local snapshot = require "snapshot"









local function test2()
	for i = 1, 1000 do
		for i = 1,10 do
			local t = {}
			local t = {}
			local t = {}
			local t = {}
		end
	end
end


local function test1(a,b)
	local str=""
	for i = 1,1024 do
		local c = a + b
		local t = {}
		table.insert(t,c)

		str = str.."asdfa2222222222adadsfasdfasfasdfasdfsfadsf"
		test2()
	end
	
end

local report = {}

local call_info = {name = "root",childs = {}}

local function hook(ev)
	local info = debug.getinfo(2,"Snl")
	local name = string.format("%s:%s",info.source,info.name)
	if name == "=[C]:sethook" then
		return
	end

	local func_info = report[name]
	if not func_info then
		func_info = {name = name,total_time = 0,start_time = -1,count = 0,childs = {},start_mem = -1,total_mem = 0}
		report[name] = func_info
	end
	if ev == "call" then
		func_info.start_time = util.time()
		func_info.start_mem = math.modf(collectgarbage("count"))
		func_info.count = func_info.count + 1

		local parent_func_info = report[call_info.name]
		if parent_func_info then
			parent_func_info.childs[name] = (parent_func_info.childs[name] or 0) + 1
		end

		local ci = {name = name,childs = {},parent = call_info}
		call_info.childs[name] = (call_info.childs[name] or 0) + 1
		call_info = ci
	else
		func_info.total_mem = func_info.total_mem + math.modf(collectgarbage("count")) - func_info.start_mem
		func_info.total_time = func_info.total_time + util.time() - func_info.start_time

		call_info = call_info.parent
	end

end




event.fork(function ()
	collectgarbage("stop")
	debug.sethook(hook,"cr")
	test1(1,2)
	debug.sethook(nil)
	collectgarbage("restart")

	local list = {}
	for name,info in pairs(report) do
		table.insert(list,{name = name,time = info.total_time,mem = info.total_mem,count = info.count})
	end
	table.sort(list,function (l,r)
		return l.mem > r.mem
	end)

	for _,info in ipairs(list) do
		print(string.format("%- 30s %- 10d %- 20f %- 20f",info.name,info.count,info.time * 10 / info.count,info.mem/info.count))
	end

	local dump = snapshot()
	table.print(dump)
end)
