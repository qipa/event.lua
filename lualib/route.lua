local protocol = require "protocol"
local event = require "event"
local import = require "import"

local _M = {}


function _M.dispatch(file,method,...)

	local lm = import.get_module(file)
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
	if not protocol.handler[name] then
		event.error(string.format("no such id:%d proto:%s ",message_id,name))
		return
	end
	protocol.handler[name](...,message)
end


return _M