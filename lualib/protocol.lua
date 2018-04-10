local parser = require "protocolparser"
local dump = require "dump.core"
local protocolcore = require "protocolcore"
local util = require "util"


local _ctx

local _M = setmetatable({},{__gc = function ()
	if _ctx then
		protocolcore.release(_ctx)
	end
end})

local function replace_field(info)
	if info.fields ~= nil then
		local new_fields = {}
		for name,field_info in pairs(info.fields) do
			new_fields[field_info.index+1] = {type = field_info.type,array = field_info.array,type_name = field_info.type_name,name = field_info.name}
		end
		info.fields = new_fields
	end
end

local function remake_field(children)
	for _,info in pairs(children) do
		replace_field(info)
		if info.children ~= nil then
			remake_field(info.children)
		end
	end
end

local function replace_sub_protocol(info,func)
	for _,info in pairs(info) do
		if info.children ~= nil then
			replace_sub_protocol(info.children,func)
		end
		if info.fields ~= nil then
			func(info) 
		end
	end
end

_M.encode = {}
_M.decode = {}

local _protocol_raw = {}

function _M.parse(file)
	local fullfile = string.format("%s.protocol",file)
	local tbl = parser.parse("./protocol/",fullfile)
	remake_field(tbl.root.children)
	replace_sub_protocol(tbl.root.children,function (protocol)
		for _,field in pairs(protocol.fields) do
			if field.type == 4 then
				local info
				if protocol.children ~= nil then
					info = protocol.children[field.type_name]
					if info == nil then
						info = tbl.root.children[field.type_name]
					end
				else
					info = tbl.root.children[field.type_name]
				end
				assert(info ~= nil,string.format("no such protocol %s",field.type_name))
				field.fields = info.fields
			end
		end
	end)

	for name,proto in pairs(tbl.root.children) do
		if proto.file ~= fullfile then
			tbl.root.children[name] = nil
		end
	end

	local str = dump.pack_order(tbl.root.children)
	_protocol_raw[file] = str
end

function _M.load()
	local meta = {}
	for _,raw in pairs(_protocol_raw) do
		local info = load("return"..raw)()
		for name,proto in pairs(info) do
			table.insert(meta,{name = name,proto = proto})
		end
	end
	table.sort(meta,function (l,r)
		return l.name < r.name
	end)

	local encode = {}
	local decode = {}

	local name_map = {}
	_ctx = protocolcore.new()
	for i,info in ipairs(meta) do
		_ctx:load(i,info.name,info.proto)
		name_map[info.name] = i

		encode[i] = function (tbl)
			return _ctx:encode(_ctx,i,tbl)
		end

		decode[i] = function (...)
			return _ctx:decode(_ctx,i,...)
		end
	end

	_M.encode = encode
	_M.decode = decode
end

function _M.dump(id)
	if not id then
		local map = _ctx:dump()
		for name,id in pairs(map) do
			_M.dump(id)
		end
		return
	end
	local map = _ctx:detail(id)
	table.print(map)
end

function _M.dumpfile()
	local meta = {}
	for _,raw in pairs(_protocol_raw) do
		local info = load("return"..raw)()
		for name,proto in pairs(info) do
			table.insert(meta,{name = name,proto = proto})
		end
	end
	table.sort(meta,function (l,r)
		return l.name < r.name
	end)

	local FILE = io.open("tmp/protocol.dump","w")
	FILE:write(dump.tostring(meta))
	FILE:close()
end

return _M