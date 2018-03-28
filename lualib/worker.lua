local event = require "event"
local route = require "route"
local worker = require "worker.core"

local _M = {}

local _session_callback = {}

local _core
function _M.dispatch(core)
	_core = core
	core:dispatch(function (source,session,data,size)
		local message = table.decode(data,size)
		if message.ret then
			if _session_callback[session] then
				_session_callback[session](table.unpack(message.args))
			else
				event.wakeup(session,table.unpack(message.args))
			end
		else
			local ok,result = xpcall(route.dispatch,debug.traceback,message.file,message.method,table.unpack(message.args))
			if not ok then
				event.error(result)
			end
			if session ~= 0 then
				core:push(source,session,table.tostring({ret = true,args = {result}}))
			end
		end
	end)
end

function _M.main_send(target,file,method,...)
	worker.push(target,0,table.tostring({file = file,method = method,args = {...}}))
end

function _M.main_create(fd,args)
	return worker.create(fd,args)
end

function _M.send(target,file,method,...)
	_core:push(target,0,table.tostring({file = file,method = method,args = {...}}))
end

function _M.send_back(target,file,method,args,callback)
	local session = event.gen_session()
	_session_callback[session] = callback
	_core:push(target,session,table.tostring({file = file,method = method,args = args}))
end

function _M.call(target,file,method,...)
	local session = event.gen_session()
	_core:push(target,session,table.tostring({file = file,method = method,args = {...}}))
	return event.wait(session)
end

function _M.send_mail(args)
	_core:send_mail(0,table.encode(args))
end





return _M