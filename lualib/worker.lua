local event = require "event"
local route = require "route"
local worker = require "worker.core"

local _M = {}

local _session_callback = {}

--for creator
local _maibox
local _mailbox_fd
local _worker_group = {}

--for worker
local _worker_userdata

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

function _M.create(args)
	if not _maibox then
		_maibox,_mailbox_fd = event.mailbox(function (mailbox,source,session,data,size)
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
	local pid = worker.create(_mailbox_fd,args)
	table.insert(_worker_group,pid)
	return pid
end

function _M.join()
	for _,pid in pairs(_worker_group) do
		worker.join(pid)
	end
end

function _M.dispatch(worker_ud)
	_worker_userdata = worker_ud
	_worker_userdata:dispatch(function (source,session,data,size)
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
					_worker_userdata:send_mail(session,table.tostring({ret = true,args = result}))
				else
					_worker_userdata:push(source,session,table.tostring({ret = true,args = result}))
				end
			end
		end
	end)
end

function _M.quit()
	_worker_userdata:quit()
end

function _M.send_worker(target,file,method,args)
	_worker_userdata:push(target,0,table.tostring({file = file,method = method,args = args}))
end

function _M.call_worker(target,file,method,args,func)
	local session = event.gen_session()
	_worker_userdata:push(target,session,table.tostring({file = file,method = method,args = args}))
	if func then
		_session_callback[session] = callback
		return
	end
	return event.wait(session)
end

function _M.send_mail(file,method,args)
	_worker_userdata:send_mail(0,table.tostring({file = file,method = method,args = args}))
end

function _M.call_mail(file,method,args,func)
	local session = event.gen_session()
	_worker_userdata:send_mail(session,table.tostring({file = file,method = method,args = args}))
	if func then
		_session_callback[session] = func
		return
	end
	return event.wait(session)
end


return _M