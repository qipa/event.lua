local protocol = require "protocol"
local co_core = require "co.core"
local monitor = require "monitor"
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

	-- co_core.start()
	local result = func(...)
	-- local diff = co_core.stop()
	-- monitor.report_diff(file,method,diff)
	return result
end

function _M.dispatch_client(message_id,data,size,...)
	local name,message = protocol.decode[message_id](data,size)
	if not protocol.handler[name] then
		event.error(string.format("no such id:%d proto:%s ",message_id,name))
		return
	end

	monitor.report_input(protocol,message_id,size)

	-- co_core.start()
	protocol.handler[name](...,message)
	-- local diff = co_core.stop()
	-- monitor.report_diff("protocol",message_id,diff)
end


return _M