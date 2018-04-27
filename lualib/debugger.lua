local import = require "import"
local util = require "util"
local dump_core = require "dump.core"

local ldb = {}

function ldb:new()
	local obj = setmetatable({},{__index = self})
	obj.breakpoint = {}
	obj.breakpoint_index = 1
	obj.step = false
	obj.step_next = nil
	obj.start = false
	obj.source_ctx = {}
	obj.current_frame = nil
	obj.current_source = nil
	obj.current_line = nil
	return obj
end

function ldb:breakpoint_here(file,line)
	local script_ctx = import.get_script_ctx()
	local look_like = {}
	for name,info in pairs(script_ctx) do
		local _,over = name:find("%.")
		if name:sub(over+1) == file then
			local source = string.format("@user.%s",info.name)
			local bp_line_info = self.breakpoint[source]
			if not bp_line_info then
				bp_line_info = {}
				self.breakpoint[source] = bp_line_info
			end
			bp_line_info[line] = self.breakpoint_index
			self.breakpoint_index = self.breakpoint_index + 1
			table.print(self.breakpoint)
			return true,source
		else
			local start = name:find(file)
			if start then
				table.insert(look_like,name)
			end
		end
	end
	return false,look_like
end

function ldb:breakpoint_delete(index)
	local delete_index = tonumber(index)
	for source,info in pairs(self.breakpoint) do
		local delete_line
		for line,index in pairs(info) do
			if index == delete_index then
				delete_line = line
				break
			end
		end
		if delete_line then
			info[delete_line] = nil
			return true,source,delete_line
		end
	end
	return false
end

function ldb:log_source(line,range)
	if not line then
		line = self.current_line
	end

	local name = self.current_source:sub(7)
	if not self.source_ctx[name] then
		local content = {}
		local script_ctx = import.get_script_ctx()
		local script_info = script_ctx[name]
		local fd = io.open(script_info.fullfile,"r")
		local index = 1
		for line in fd:lines() do
			table.insert(content,string.format("%d\t%s",index,line))
			index = index + 1
		end
		self.source_ctx[name] = content
	end
	

	local str = {}
	for i = tonumber(line) - range,tonumber(line) + range do
		if i >= 0 and i <= #self.source_ctx[name] then
			table.insert(str,self.source_ctx[name][i])
		end
	end
	return str
end

function ldb:getlocal(from,to,name)
	local index = from or 1
	if to ~= nil and index > to then
		return false
	end

	local search = false
	if name then
		search = true
	end

	local result = {}

	while true do
		local k,v = debug.getlocal(self.current_frame + 1,index)
		if not k then
			break
		end

		if search then
			if name == k then
				local tbl = {}
				tbl[k] = v
				table.insert(result,tbl)
				break
			end
		else
			if k ~= "(*temporary)" then
				local tbl = {}
				tbl[k] = v
				table.insert(result,tbl)
			end
		end

		index = index + 1
		if to and index > to then
			break
		end
	end

	if next(result) == nil then
		return false
	end
	return true,dump_core.tostring(result)
end

function ldb:getupvalue(name)
	local info = debug.getinfo(self.current_frame + 1,"f")
	
	local result = {}
	local index = 1
	while true do
		local k,v = debug.getupvalue(info.func,index)
		if k == name then
			result[k] = v
			break
		end
		if not k then
			break
		end
		index = index + 1
	end

	if next(result) == nil then
		return false
	end
	return true,dump_core.tostring(result)
end

function ldb:getframestack()
	local traceback = debug.traceback(coroutine.running(),nil,self.current_frame + 1)
	local frame = {}
	local index = -1
	for line in traceback:gmatch("(.-)\n") do
		if index >= 0 then
			table.insert(frame,{index = index,line = line})
		end
		index = index + 1
	end
	return frame
end

function ldb:search_script(str)
	local script_ctx = import.get_script_ctx()
	local result = {}
	for name in pairs(script_ctx) do
		local _,over = name:find("%.")
		name = name:sub(over+1)
		if name:match(str) then
			table.insert(result,name)
		end
	end
	return result
end

local _debugger 


local CMD = {}

local function completion(debugger,str)
	local ch = str:sub(1,1)
	if ch == "i" then
		local str = str:sub(2)
		if str then
			local offset = str:find("%S")
			if offset then
				local ch = str:sub(offset,offset + 1)
				if ch == "b" then
					return {"i breakpoint"}
				elseif ch == "a" then
					return {"i args"}
				elseif ch == "l" then
					return {"i local"}
				end
			else
				local ch = str:sub(1,1)
				if ch == " " then
					return {"i breakpoint","i args","i local"}
				end 
			end
		end
	elseif ch == "b" then
		local str = str:sub(2)
		if str then
			local offset = str:find("%S")
			if offset then
				local str = str:sub(offset)
				local result = debugger:search_script(str)
				if #result ~= 0 then
					local list = {}
					for _,name in pairs(result) do
						table.insert(list,string.format("b %s",name))
					end
					return list
				end
 			end
		end
	end
end

local function command_loop(debugger)
	while true do
		local line = util.readline(string.format("%s@%d>>",debugger.current_source,debugger.current_line),function (str)
			return completion(debugger,str)
		end)
		local args = {}
		local cmd = nil
		for word in string.gmatch(line, "%S+") do
			if not cmd then
				cmd = word
			else
				table.insert(args,word)
			end
		end
		local func = CMD[cmd]
		if func then
			local result = func(debugger,table.unpack(args))
			if not result then
				break
			end
		end
	end
end



local function hook(event,line)
	if event == "line" then
		local info = debug.getinfo(2,"Snl")
		local short = info.source:sub(1,5)
		if short ~= "@user" then
			return
		end
		_debugger.current_frame = 4
		_debugger.current_source = info.source
		_debugger.current_line = info.currentline
		if not _debugger.start then
			_debugger.start = true
			_debugger:breakpoint_here(info.source,info.currentline)
			command_loop(_debugger)
		else
			if _debugger.step then
				_debugger.step = false
				command_loop(_debugger)
			else
				if _debugger.step_next then
					if info.source == _debugger.step_next.source and info.name == _debugger.step_next.name then
						_debugger.step_next = nil
						command_loop(_debugger)
						return
					end
				end
				local bp_info = _debugger.breakpoint[info.source]
				if not bp_info then
					return
				end
				if bp_info[info.currentline] then
					command_loop(_debugger)
				end
			end
		end
	end
end

function CMD.p(debugger,name)
	if not name then
		print("invalid input")
		return
	end

	local ok,result = debugger:getlocal(nil,nil,name)
	if ok then
		print(result)
	else
		local ok,result = debugger:getupvalue(name)
		if ok then
			print(result)
		else
			print(string.format("no such var:%s found",name))
		end
	end
	return true
end

function CMD.q(debugger)
	debug.sethook(nil)
	_debugger = nil
	return false
end

function CMD.c(debugger)
	debugger.step_next = nil
	debugger.step = false
	return false
end

function CMD.t(debugger)
	local frame = debugger:getframestack()
	local result = {}
	for _,info in ipairs(frame) do
		table.insert(result,string.format("#%d%s",info.index,info.line))
	end
	print(table.concat(result,"\n"))
	return true
end

function CMD.b(debugger,file,line)
	local script_ctx = import.get_script_ctx()
	
	local ok,info = debugger:breakpoint_here(file,tonumber(line))
	if ok then
		print(string.format("breakpoint in %s@%d",info,line))
	else
		print(string.format("no such file:%s",file))
		if next(info) then
			print("Do you mean this?")
			print(table.concat(info,"\r\n"))
		end
	end
	return true
end

function CMD.d(debugger,index)
	if not index then
		local line = util.readline("delete all breakpoint?yes or no>>")
		if line then
			line = line:lower()
			local yes 
			if line == "y" or line == "yes"  then
				yes = true
			elseif line == "n" or line == "no" then
				yes = false
			end
		end
		debugger.breakpoint = {}
		print(dump_core.tostring(debugger.breakpoint))
	else
		local ok,source,line = debugger:breakpoint_delete(index)
		if ok then
			print(string.format("delete %s.lua:%d",source:sub(7),line))
		end
	end
	return true
end

function CMD.n(debugger)
	local info = debug.getinfo(debugger.current_frame,"lnS")
	debugger.step_next = {source = info.source,name = info.name}
	local str = debugger:log_source(debugger.current_line+1,0)
	print(table.concat(str,"\r\n"))
	if debugger.current_line + 1 > info.lastlinedefined then
		debugger.step_next = nil
		local info = debug.getinfo(debugger.current_frame+1,"lnS")
		local short = info.source:sub(1,5)
		if short == "@user" then
			if info.currentline + 1 <= info.lastlinedefined then
				debugger.step_next = {source = info.source,name = info.name}
			end
		end
	end

	return false
end

function CMD.s(debugger)
	local info = debug.getinfo(debugger.current_frame,"lnS")
	local str = debugger:log_source(debugger.current_line+1,0)
	print(table.concat(str,"\r\n"))
	debugger.step = true
	if debugger.current_line + 1 > info.lastlinedefined then
		debugger.step = false
	end

	return false
end

function CMD.l(debugger,line)
	local str = debugger:log_source(line,3)
	print(table.concat(str,"\r\n"))
	return true
end

function CMD.i(debugger,args)
	if args == "b" or args == "breakpoint" then
		local result = {}
		for source,info in pairs(debugger.breakpoint) do
			for line,index in pairs(info) do
				local file = string.format("%s.lua:%d",source:sub(7),line)
				table.insert(result,{index = index,source = file})
			end
		end
		table.sort(result,function (l,r)
			return l.index < r.index
		end)
		print(dump_core.tostring(result))
	elseif args == "args" then
		local info = debug.getinfo(debugger.current_frame,"u")
		local ok,result = debugger:getlocal(1,info.nparams)
		if ok then
			print(result)
		end

	elseif args == "local" then
		local info = debug.getinfo(debugger.current_frame,"u")
		local index = info.nparams
		if index == 0 then
			index = 1
		else
			index = index + 1
		end
		local ok,result = debugger:getlocal(index)
		if ok then
			print(result)
		end
	else
		print("invalid input:i b|breakpoint|args|local")
	end
	return true
end

function CMD.f(debugger,index)
	index = tonumber(index)
	if index == nil or index < 0 then
		return true
	end
	local frame = debugger:getframestack()
	if index >= #frame then
		return true
	end
	local info = frame[index+1]
	if info.line:sub(2,5) ~= "user" then
		print("only can swith to frame load by import")
	else
		print("switch to frame:")
		print(info.line)
		debugger.current_frame = 4 + index
		local info = debug.getinfo(debugger.current_frame,"Snl")
		debugger.current_source = info.source
		debugger.current_line = info.currentline
	end

	return true
end

function CMD.h(debugger)
	print("[h]elp                         show this help.")
	print("[s]tep                         run current line and stop again.")
	print("[n]ext                         run current line,ignore step into other func and stop again.")
	print("[c]ontinue                     run till next breakpoint.")
	print("[l]list                        list source code around current line.")
	print("[l]list [line]                 list source code around [line].")
	print("[p]rint name                   print local var or param.")
	print("[q]uit                         quit debugger.")
	print("[t]race                        show a backtrace.")
	print("[f]rame index                  switch frame.")
	print("[b]reak file line              add a breakpoint in file@line.")
	print("[d]elete [index]               delete breakpoint with index.")
	print("[i]nfo b|breakpoint|args|local show breakpoint,args or local vars")
	return true
end

return function (file,line)
	if _debugger then
		error("debugger already running")
	end
	_debugger = ldb:new()
	if file and line then
		local ok,info = _debugger:breakpoint_here(file,line)
		if not ok then
			print(string.format("add break point failed,no such file:%s",file))
		end
	end

	debug.sethook(hook,"l")
end