
local _M = {}


function _M.dispatch(file,method,...)

	local lm = import(file)
	if not lm then
		error(string.format("no such file:%s",file))
	end

	local func = lm[method]
	if not func then
		error(string.format("no such method:%s",method))
	end
	return func(...)
end

function _M.dispatch_client(message_id,data,size,...)
	local name,message = protocol.decode[message_id](data,size)
	protocol.handler[name](...,message)
end


return _M