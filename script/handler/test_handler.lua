local worker = require "worker"
local event = require "event"
local util = require "util"
local mysql_core = require "mysql.core"

local _M = {}

local function fuck_debugger(a)
	a = a + 1
	print(a)
end


function test(a,b)

	fuck_debugger(a)
end

local count = 0
function test_rpc(args)
	count = count + 1
	if count %10000 == 0 then
		print(count)
	end
	return "ok"
end


function test_thread_rpc0(args)
	local now = util.time()
	util.time_diff("diff",function ()
		local count = 1024 * 1024 * 10
		for i = 1,count do
			worker.call_worker(1,"handler.test_handler","test_thread_rpc1",{fuck = "you"})
		end
	end)
	return "rpc ok"
end

function test_thread_rpc1(args)
	return "ok"
end

function mail_rpc(args)
	table.print(args,"mail_rpc")
	return "mail ok"
end

function _M.test_thread_rpc_(source,session,args)
	-- print(session)
	-- worker.push(0,env.worker_id,session,table.tostring({ret = true,args = {"ok"}}))
end

function _M.getaddrinfo(args)
	return util.getaddrinfo(args.host,0)
end

local mysql,mysql_err

function execute(sql)
	local event_core = require "ev.core"
	print(event_core.test(),env.thread_id)
	-- if not mysql then
	-- 	mysql,err = mysql_core.connect("127.0.0.1","root","2444cc818a3bbc06","event_test",3306)
	-- 	if not mysql then
	-- 		return {false,err}
	-- 	end
	-- 	print(mysql)
	-- end
	-- return mysql:execute(sql)
end

return _M