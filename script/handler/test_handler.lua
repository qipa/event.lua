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


function test_thread_rpc(args)
	table.print(args)
	worker.send_mail(0,{hello = "world"})
	count = count + 1
	if count %10000 == 0 then
				print(count)
			end
	return "ok1"
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