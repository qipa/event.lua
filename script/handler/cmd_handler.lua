local event = require "event"
local import = require "import"
local model = require "model"
local helper = require "helper"
local dump_core = require "dump.core"
local server_manager = import.import "module.server_manager"

function stop()
	event.breakout()
end

function mem()
	if env.name == "server/world" then
		local result = server_manager:broadcast("handler.cmd_handler","mem")
		local mem = collectgarbage("count")
		result[env.dist_id] = string.format("lua mem:%fkb,c mem:%fkb",mem,helper.allocated() / 1024)
		return result
	end
	local mem = collectgarbage("count")
	return string.format("lua mem:%fkb,c mem:%fkb",mem,helper.allocated() / 1024)
end

function gc()
	event.clean()
	collectgarbage("collect")
	helper.free()
	if env.name == "server/world" then
		local result = server_manager:broadcast("handler.cmd_handler","gc")
		local mem = collectgarbage("count")
		result[env.dist_id] = string.format("lua mem:%fkb,c mem:%fkb",mem,helper.allocated() / 1024)
		return result
	end
	local mem = collectgarbage("count")
	return string.format("lua mem:%fkb,c mem:%fkb",mem,helper.allocated() / 1024)
end

function mem_dump()
	helper.heap.dump("dump")
	return "ok"
end

function reload(_,file)

	if env.name == "server/world" then
		local result = server_manager:broadcast("handler.cmd_handler","reload",file)
		local list
		if file[1] then
			import.reload(file[1])
			list = {}
			table.insert(list,file[1])
		else
			list = import.auto_reload()
		end
		result[env.dist_id] = list
		return result

	end
	local list
	if file[1] then
		import.reload(file[1])
		list = {}
		table.insert(list,file[1])
	else
		list = import.auto_reload()
	end
	return list
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
