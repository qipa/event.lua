
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
local profiler = require "profiler.core"
local worker = require "worker"
local monitor = require "monitor"

table.print = util.dump
table.encode = serialize.pack
table.decode = serialize.unpack
table.tostring = serialize.tostring
-- table.encode = dump.pack
-- table.decode = dump.unpack
-- table.tostring = dump.tostring

string.split = util.split
string.copy = util.clone_string

_G.MODEL_BINDER = model.register_binder
_G.MODEL_VALUE = model.register_value
_G.tostring = util.tostring
_G.import = import.import
_G.debugger = debugger
_G.debugger_ctx = {}

import.import "module.object"

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

local list = name:split("/")

local command = string.format("%s@%07d",list[#list],env.uid)
util.thread_name(command)

if main then
	local _G_protect = {}
	function _G_protect.__newindex(self,k,v)
		rawset(_G,k,v)
		print(string.format("%s:%s add to _G",k,v))
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
end

monitor.start()

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


