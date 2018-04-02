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
	table.print(tbl)
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
			meta[name] = proto
		end
	end

	_ctx = protocolcore.load(meta)
	local map = protocolcore.dump_all(_ctx)

	local encode = {}
	local decode = {}
	for name in pairs(meta) do
		encode[name] = function (tbl)
			local info = meta[name]
			return protocolcore.encode(_ctx,map[name],tbl)
		end

		decode[name] = function (...)
			local info = meta[name]
			return protocolcore.decode(_ctx,map[name],...)
		end
	end
	_M.encode = encode
	_M.decode = decode
end

function _M.dump(name)
	if not name then
		local map = protocolcore.dump_all(_ctx)
		for name in pairs(map) do
			_M.dump(name)
		end
		return
	end
	local map = protocolcore.dump_all(_ctx)
	table.print(protocolcore.dump(_ctx,map[name]))
end

function _M.dumpfile(path)
	for name,raw in pairs(_protocol_raw) do
		local fd = io.open(string.format("%s/%s.dump",path,name),"w")
		fd:write(raw)
		fd:close()
	end
end

return _M