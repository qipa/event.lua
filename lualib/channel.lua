local co_core = require "co.core"
local route = require "route"
local event = require "event"
local monitor = require "monitor"
local util = require "util"

local WARN_FLOW = 1024 * 1024
local channel = {}

local xpcall = xpcall
local tinsert = table.insert
local tunpack = table.unpack
local setmetatable = setmetatable
local pairs = pairs

function channel:inherit()
	local children = setmetatable({},{__index = self})
	return children
end

function channel:new(buffer,addr)
	local ctx = setmetatable({},{__index = self})
	ctx.buffer = buffer
	ctx.addr = addr or "unknown"
	ctx.session_ctx = {}
	ctx.threhold = WARN_FLOW
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
			tinsert(list,session)
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
	local ok,result = xpcall(route.dispatch,debug.traceback,file,method,channel,args)
	if not ok then
		event.error(result)
	end
	if session ~= 0 then
		if not ok then
			channel:ret(session,false,result)
		else
			channel:ret(session,true,result)
		end
	end
end

local function diff_method(channel,session,file,method,args)
	co_core.start()

	call_method(channel,session,file,method,args)

	local diff = co_core.stop()

	monitor.report_diff(file,method,diff)
	
end

function channel:dispatch(message,size)
	if message.ret then
		local call_ctx = self.session_ctx[message.session]
		if call_ctx.callback then
			call_ctx.callback(message.args)
		else
			event.wakeup(message.session,tunpack(message.args))
		end
		self.session_ctx[message.session] = nil
	else
		monitor.report_input(message.file,message.method,size)
		event.fork(diff_method,self,message.session,message.file,message.method,message.args)
	end
end

function channel:data(data,size)
	local message = table.decode(data,size)
	self:dispatch(message,size)
end

local function pack_table(tbl)
	-- local str = table.tostring(tbl)
	-- local pat = string.format("I4c%d",str:len())
	-- return string.pack(pat,str:len()+4,str)
	local ptr,size = table.encode(tbl)
	return util.rpc_pack(ptr,size)
end

function channel:write(...)
	local ok,total = self.buffer:write(...)
	if ok then
		if total >= self.threhold then
			local mb = math.modf(total/WARN_FLOW)
			event.error(string.format("channel:%s more than %dmb data need to send out",self.addr,mb))
			self.threhold = self.threhold + WARN_FLOW
		else
			if total < self.threhold / 2 then
				self.threhold = self.threhold - WARN_FLOW
				if self.threhold < WARN_FLOW then
					self.threhold = WARN_FLOW
				end
			end
		end
	end
end

function channel:send(file,method,args,callback)
	local session = 0
	if callback then
		session = event.gen_session()
	end
	local ptr,size = pack_table({file = file,method = method,session = 0,args = args})
	self:write(ptr,size)
	
	monitor.report_output(file,method,size)

	if session ~= 0 then
		self.session_ctx[session] = {callback = callback}
	end
end

function channel:call(file,method,args)
	local session = event.gen_session()
	self.session_ctx[session] = {}
	local ptr,size = pack_table({file = file,method = method,session = session,args = args})
	self:write(ptr,size)

	monitor.report_output(file,method,size)

	local ok,value = event.wait(session)
	if not ok then
		error(value)
	end
	return value
end

function channel:ret(session,...)
	local ptr,size = pack_table({ret = true,session = session,args = {...}})
	self:write(ptr,size)
end

function channel:close()
	self.buffer:close(false)
end

function channel:close_immediately()
	self.buffer:close(true)
	self:disconnect()
end

return channel