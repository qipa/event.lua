local event = require "event"
local route = require "route"
local worker = require "worker.core"

local _M = {}

local _session_callback = {}

local _maibox
local _core
function _M.dispatch(core)
	_core = core
	core:dispatch(function (source,session,data,size)
		local message = table.decode(data,size)
		if message.ret then
			if _session_callback[session] then
				_session_callback[session](message.args)
			else
				event.wakeup(session,message.args)
			end
		else
			local ok,result = xpcall(route.dispatch,debug.traceback,message.file,message.method,message.args)
			if not ok then
				event.error(result)
			end
			if session ~= 0 then
				if source < 0 then
					_core:send_mail(session,table.tostring({ret = true,args = result}))
				else
					core:push(source,session,table.tostring({ret = true,args = result}))
				end
			end
		end
	end)
end

function _M.main_send(target,file,method,args)
	worker.push(target,0,table.tostring({file = file,method = method,args = args}))
end

function _M.main_call(target,file,method,args,func)
	local session = event.gen_session()
	worker.push(target,session,table.tostring({file = file,method = method,args = args}))
	if func then
		_session_callback[session] = func
		return
	end
	return event.wait(session)
end

function _M.main_create(args)
	if not _maibox then
		_maibox = event.mailbox(function (mailbox,source,session,data,size)
			local message = table.decode(data,size)
			if message.ret then
				if _session_callback[session] then
					_session_callback[session](message.args)
				else
					event.wakeup(session,message.args)
				end
			else
				local ok,result = xpcall(route.dispatch,debug.traceback,message.file,message.method,message.args)
				if not ok then
					event.error(result)
				end
				if session ~= 0 then
					worker.push(source,session,table.tostring({ret = true,args = result}))
				end
			end
		end)
	end

	return worker.create(_maibox:fd(),args)
end

function _M.send_worker(target,file,method,args)
	_core:push(target,0,table.tostring({file = file,method = method,args = args}))
end

function _M.call_worker(target,file,method,args,func)
	local session = event.gen_session()
	_core:push(target,session,table.tostring({file = file,method = method,args = args}))
	if func then
		_session_callback[session] = callback
		return
	end
	return event.wait(session)
end

function _M.send_mail(file,method,args)
	_core:send_mail(0,table.tostring({file = file,method = method,args = args}))
end

function _M.call_mail(file,method,args,func)
	local session = event.gen_session()
	_core:send_mail(session,table.tostring({file = file,method = method,args = args}))
	if func then
		_session_callback[session] = func
		return
	end
	return event.wait(session)
end





return _M