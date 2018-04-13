
local _M = {}


local _binder_ctx = {}
local _value_ctx = {}


function _M.register_binder(name,...)
	local keys = {...}
	assert(#keys ~= 0)

	assert(_binder_ctx[name] == nil,string.format("name:%s already register",name))
	_binder_ctx[name] = {}
	
	_M[string.format("fetch_%s",name)] = function ()
		local same_name = _binder_ctx[name]
		if same_name and same_name[keys[1]] then
			local result = {}
			for _,info in pairs(same_name[keys[1]]) do
				table.insert(result,info.value)
			end
			return result
		end	
	end

	local binder_map = {}

	for _,key in pairs(keys) do
		if not _binder_ctx[name][key] then
			_binder_ctx[name][key] = {}
		end

		_M[string.format("fetch_%s_with_%s",name,key)] = function (k)
			local same_name = _binder_ctx[name]
			local same_k = same_name[key]
			if not same_k[k] then
				return
			end
			return same_k[k].value
		end

		_M[string.format("bind_%s_with_%s",name,key)] = function (k,value)
			local same_name = _binder_ctx[name]
			local same_k = same_name[key]
			if not same_k then
				same_name[key] = {}
				same_k = same_name[key]
			end
			same_k[k] = {time = os.time(),value = value}
		end

		_M[string.format("unbind_%s_with_%s",name,key)] = function (k)
			local same_name = _binder_ctx[name]
			local same_k = same_name[key]
			same_k[k] = nil
		end
	end
end

function _M.register_value(name)
	_M[string.format("set_%s",name)] = function (value)
		_value_ctx[name] = {time = os.time(),value = value}
	end

	_M[string.format("get_%s",name)] = function ()
		local ctx = _value_ctx[name]
		if ctx then
			return ctx.value
		end
		return
	end

	_M[string.format("delete_%s",name)] = function ()
		_value_ctx[name] = nil
	end
end

function _M.count_model()
	local value_report = {}
	for name,ctx in pairs(_value_ctx) do
		local now = math.modf(ctx.now / 100)
		table.insert(value_report,string.format("born:%s,value:%s",os.date("%Y-%m-%d %H:%M:%S",now),ctx.value))
	end

	local binder_report = {}
	for name,binder in pairs(_binder_ctx) do
		local report = {}
		for key,ctx in pairs(binder) do
			local key_report = {}
			for k,v in pairs(ctx) do
				local now = math.modf(v.now / 100)
				table.insert(key_report,string.format("born:%s,k:%s,v:%s",os.date("%Y-%m-%d %H:%M:%S",now),k,v.value))
			end
			report[key] = key_report
		end
		binder_report[name] = report
	end

	return table.tostring(value_report),table.tostring(binder_report)
end


return _M