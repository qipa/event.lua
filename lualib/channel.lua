local route = require "route"
local event = require "event"

local channel = {}

function channel:inherit()
	local children = setmetatable({},{__index = self})
	return children
end

function channel:new(buffer,addr)
	local ctx = setmetatable({},{__index = self})
	ctx.buffer = buffer
	ctx.addr = addr or "unknown"
	ctx.session_ctx = {}
	return ctx
end

function channel:init(...)

end

function channel:attach(buffer)
	self.buffer = buffer
end

function channel:disconnect()
	local list = {}
	for session,ctx in pairs(self.session_ctx) do
		if not ctx.callback then
			table.insert(list,session)
		end
	end

	table.sort(list,function (l,r)
		return l < r
	end)

	for _,session in pairs(list) do
		event.wakeup(session,false,"buffer closed")
	end
	self.session_ctx = {}
end

function channel:read(num)
	return self.buffer:read(num)
end

function channel:read_line()
	return self.buffer:read_line()
end

function channel:read_util(sep)
	return self.buffer:read_util(sep)
end

local function call_method(channel,session,file,method,args)
	local ok,result = xpcall(route.dispatch,debug.traceback,file,method,table.unpack(args))
	if not ok then
		event.error(result)
	end
	if session ~= 0 then
		if not ok then
			channel:ret(session,false,result)
		else
			channel:ret(session,result)
		end
	end
end

function channel:dispatch(message)
	if message.ret then
		local call_ctx = self.session_ctx[message.session]
		if call_ctx.callback then
			call_ctx.callback(table.unpack(message.args))
		else
			event.wakeup(message.session,table.unpack(message.args))
		end
		self.session_ctx[message.session] = nil
	else
		event.fork(call_method,self,message.session,message.file,message.method,message.args)
	end
end

function channel:data(data,size)
	local message = table.decode(data,size)
	self:dispatch(message)
end

local function pack_table(tbl)
	local str = table.tostring(tbl)
	local pat = string.format("I4c%d",str:len())
	return string.pack(pat,str:len()+4,str)
end

function channel:write(...)
	self.buffer:write(...)
end

function channel:send(file,method,...)
	local str = pack_table({file = file,method = method,session = 0,args = {...}})
	self:write(str)
end

function channel:call(file,method,...)
	local session = event.gen_session()
	self.session_ctx[session] = {}
	local str = pack_table({file = file,method = method,session = session,args = {...}})
	self:write(str)

	local ok,err = event.wait(session)
	if not ok then
		error(err)
	end
	return ok
end

function channel:send_callback(file,method,args,callback)
	local session = event.gen_session()
	self.session_ctx[session] = {callback = callback}
	local str = pack_table({file = file,method = method,session = session,args = args})
	self:write(str)
end

function channel:ret(session,...)
	local str = pack_table({ret = true,session = session,args = {...}})
	self:write(str)
end

function channel:close()
	self.buffer:close()
end

function channel:close_immediately()
	self.buffer:close_immediately()
end

return channel