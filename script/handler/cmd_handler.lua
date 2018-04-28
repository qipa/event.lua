local event = require "event"
local import = require "import"
local model = require "model"
local helper = require "helper"
local monitor = require "monitor"
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

function flush()
	if env.name == "server/world" then
		local result = server_manager:broadcast("handler.cmd_handler","flush")
		local world_server = import.import "module.world_server"
		world_server:flush()
		result[env.dist_id] = "ok"
		return result
	elseif env.name == "server/agent" then
		local agent_server = import.import "module.agent_server"
		agent_server:flush()
	elseif env.name == "server/login" then
		local login_server = import.import "module.login_server"
		login_server:flush()
	elseif env.name == "server/scene" then
		local scene_server = import.import "module.scene_server"
		scene_server:flush()
	end
	return "ok"
end

function monitor_dump()
	if env.name == "server/world" then
		monitor.dump()
		local result = server_manager:broadcast("handler.cmd_handler","monitor_dump")
		result[env.dist_id] = "ok"
		return result
	else
		monitor.dump()
	end
	return "ok"
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
