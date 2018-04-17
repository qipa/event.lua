local event = require "event"
local route = require "route"
local worker = require "worker.core"

local _M = {}

local _session_callback = {}

--for creator
local _pipe
local _pipe_fd
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
	if not _pipe then
		_pipe,_pipe_fd = event.pipe(function (pipe,source,session,data,size)
			local message = table.decode(data,size)
			if message.ret then
				if _session_callback[session] then
					if message.args then
						_session_callback[session](message.args)
					end
				else
					if message.args then
						event.wakeup(session,true,message.args)
					else
						event.wakeup(session,false,message.err)
					end
				end
			else

				local ok,result = xpcall(route.dispatch,debug.traceback,message.file,message.method,message.args)
				if session ~= 0 then
					if not ok then
						worker:push(source,session,table.tostring({ret = true,err = result}))
					else
						worker:push(source,session,table.tostring({ret = true,args = result}))
					end
				end
			end
		end)
	end
	local pid = worker.create(_pipe_fd,args)
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
				if message.args then
					_session_callback[session](message.args)
				end
			else
				if message.args then
					event.wakeup(session,true,message.args)
				else
					event.wakeup(session,false,message.err)
				end
			end
		else
			event.fork(function ()
				local ok,result = xpcall(route.dispatch,debug.traceback,message.file,message.method,message.args)
				if session ~= 0 then
					if source < 0 then
						if not ok then
							_worker_userdata:send_pipe(session,table.tostring({ret = true,err = result}))
						else
							_worker_userdata:send_pipe(session,table.tostring({ret = true,args = result}))
						end
					else
						if not ok then
							_worker_userdata:push(source,session,table.tostring({ret = true,err = result}))
						else
							_worker_userdata:push(source,session,table.tostring({ret = true,args = result}))
						end
					end
				end
			end)
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
		_session_callback[session] = func
		return
	end
	local ok,result = event.wait(session)
	if not ok then
		error(result)
	end
	return result
end

function _M.send_pipe(file,method,args)
	_worker_userdata:send_pipe(0,table.tostring({file = file,method = method,args = args}))
end

function _M.call_pipe(file,method,args,func)
	local session = event.gen_session()
	_worker_userdata:send_pipe(session,table.tostring({file = file,method = method,args = args}))
	if func then
		_session_callback[session] = func
		return
	end
	local ok,result = event.wait(session)
	if not ok then
		error(result)
	end
	return result
end


return _M