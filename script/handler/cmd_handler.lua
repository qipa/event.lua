local event = require "event"
local import = require "import"
local model = require "model"
local helper = require "helper"


function stop()
	event.breakout()
end

function mem()
	local mem = collectgarbage("count")
	return string.format("lua mem:%fkb,c mem:%fkb",mem,helper.allocated() / 1024)
end

function gc()
	event.clean()
	collectgarbage("collect")
	helper.free()
	local mem = collectgarbage("count")
	return string.format("lua mem:%fkb,c mem:%fkb",mem,helper.allocated() / 1024)
end

function mem_dump()
	helper.heap.dump("dump")
	return "ok"
end

function reload(file)
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

function ping()
	return "ok"
end

function debug(file,line)
	debugger_ctx = {file = file,line = line}
	return "ok"
end

function run(file)
	dofile(string.format("./script/%s.lua",file))
end

function dump_model()
	local value_report,binder_report = model.count_model()
	return {value_report,binder_report}
end

function top()
	return event.run_process("top -n 1")
end
