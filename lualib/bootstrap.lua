
package.path = string.format("%s;%s",package.path,"./lualib/?.lua;./script/?.lua")
package.cpath = string.format("%s;%s",package.cpath,"./.libs/?.so")

require "lfs"
local event = require "event"
local util = require "util"
local dump = require "dump.core"
local serialize = require "serialize.core"
local import = require "import"
local model = require "model"
local route = require "route"
local debugger = require "debugger"
local helper = require "helper"
local logger = require "logger"
local profiler = require "profiler.core"
local worker = require "worker"

table.print = util.dump
-- table.encode = serialize.pack
-- table.decode = serialize.unpack
-- table.tostring = serialize.tostring
table.encode = dump.pack
table.decode = dump.unpack
table.tostring = dump.tostring

string.split = util.split

_G.MODEL_BINDER = model.register_binder
_G.MODEL_VALUE = model.register_value
_G.tostring = util.tostring
_G.import = import.import
_G.debugger = debugger
_G.debugger_ctx = {}

_G.env = {}
local FILE = assert(io.open("./.env","r"))
assert(load(FILE:read("*a"),"env","text",_G.env))()

if env.config ~= nil then
	_G.config = {}
	local list = util.list_dir(env.config,true,"lua",true)
	for _,path in pairs(list) do
		local file = table.remove(path:split("/"))
		local name = file:match("%S[^%.]+")
		local data = loadfile(path)()
		config[name] = data
		print("load:",path)
	end
end

local args = {...}

local main = args[1]
local name = args[2]

local func,err = loadfile(string.format("./script/%s.lua",name),"text",_G)
if not func then
	error(err)
end

env.name = name
env.tid = util.thread_id()

local command = string.format("event@%s@%d",name,env.uid)
util.thread_name(command)

print("main",main)
if main then
	local log_path
	if env.log_path then
		local attr = lfs.attributes(env.log_path)
		if not attr then
			lfs.mkdir(env.log_path)
		end
		log_path = string.format("%s/error@%s@%s.log",env.log_path,env.name,env.uid)
	end

	local runtime_logger = logger:create("runtime",env.log_lv,log_path,5)
	event.error = function (...)
		runtime_logger:ERROR(...)
	end

	local _G_protect = {}
	function _G_protect.__newindex(self,k,v)
		rawset(_G,k,v)
		runtime_logger:WARN(string.format("%s:%s add to _G",k,v))
	end
	setmetatable(_G,_G_protect)

	if env.lua_profiler ~= nil then
		profiler.start()
	end

	if env.heap_profiler ~= nil then
		helper.heap.start(string.format("%s.%d",env.heap_profiler,env.tid))
	end

	if env.cpu_profiler ~= nil then
		helper.cpu.start(string.format("%s.%d",env.cpu_profiler,env.tid))
	end

	event.prepare()
else
	worker.dispatch(args[#args])
	event.error = function (...)
		worker.send_mail("handler.logger_handler","log_worker",{...})
	end
end

local ok,err = xpcall(func,debug.traceback,table.unpack(args,3))
if not ok then
	error(err)
end

collectgarbage("collect")
local lua_mem = collectgarbage("count")
event.error(string.format("thread:%d start,command:%s,lua mem:%fkb,c mem:%fkb",env.tid,command,lua_mem,helper.allocated()/1024))

if main then
	event.dispatch()

	worker.join()

	if env.lua_profiler ~= nil then
		profiler.stop(env.lua_profiler)
	end

	if env.heap_profiler ~= nil then
		helper.heap.dump("stop")
		helper.heap.stop()
	end

	if env.cpu_profiler ~= nil then
		helper.cpu.stop()
	end
end


