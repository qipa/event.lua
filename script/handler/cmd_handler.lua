local event = require "event"
local import = require "import"
local model = require "model"
local helper = require "helper"

local _M = {}

function _M.stop()
	event.breakout()
end

function _M.mem()
	print("mem")
	local mem = collectgarbage("count")
	return string.format("lua mem:%fkb,c mem:%fkb",mem,helper.allocated() / 1024)
end

function _M.gc()
	event.co_clean()
	collectgarbage("collect")
	helper.free()
	local mem = collectgarbage("count")
	return string.format("lua mem:%fkb,c mem:%fkb",mem,helper.allocated() / 1024)
end

function _M.mem_dump()
	helper.heap.dump("dump")
	return "ok"
end

function _M.reload(file)
	local list
	if file then
		import.reload(file)
		list = {}
		table.insert(list,file)
	else
		list = import.auto_reload()
	end
	return table.tostring(list)
end

function _M.ping()
	return "ok"
end

function _M.debug(file,line)
	debugger_ctx = {file = file,line = line}
	return "ok"
end


function _M.dump_model()
	local value_report,binder_report = model.count_model()
	return {value_report,binder_report}
end

return _M