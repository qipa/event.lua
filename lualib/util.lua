require("lfs")
local event = require "event"
local util_core = require "util.core"

local _M = {}

for method,func in pairs(lfs) do
    if type(func) == "function" then
        _M[method] = func
    end
end

for method,func in pairs(util_core) do
    if type(func) == "function" then
        _M[method] = func
    end
end

local function get_tag( t )
    local str = type(t)
    return string.sub(str, 1, 1)..":"
end

function _M.dump(data, prefix, depth, output, record)
    record = record or {}

    depth = depth or 1

    if output == nil then
        output = _G.print
    end

    local tab = string.rep("\t", depth)
    if prefix ~= nil then
        tab = prefix .. tab
    end

    if data == nil then
        output(tab.." nil")
        return
    end

    if type(data) ~= "table" then
        output(tab..get_tag(data)..tostring(data))
        return
    end

    assert(record[data] == nil)
    record[data] = true

    local count = 0
    for k,v in pairs(data) do
        local str_k = get_tag(k)
        if type(v) == "table" then
            output(tab..str_k..tostring(k).." -> ")
            _M.dump(v, prefix, depth + 1, output, record)
        else
            output(tab..str_k..tostring(k).." -> ".. get_tag(v)..tostring(v))
        end
        count = count + 1
    end

    if count == 0 then
        output(tab.." {}")
    end
end


local function get_suffix(filename)
    return filename:match(".+%.(%w+)$")
end

function _M.list_dir(path,recursive,suffix,is_path_name,r_table)
    r_table = r_table or {}

    for file in lfs.dir(path) do
        if file ~= "." and file ~= ".." then
            local f = path..'/'..file

            local attr = lfs.attributes (f)
            if attr.mode == "directory" and recursive then
                _M.list_dir(f, recursive, suffix, is_path_name,r_table)
            else
                local target = file
                if is_path_name then target = f end

                if suffix == nil or suffix == "" or suffix == get_suffix(f) then
                    table.insert(r_table, target)
                end
            end
        end
    end

    return r_table
end

function _M.split( str,reps )
    local result = {}
    string.gsub(str,'[^'..reps..']+',function ( w )
        table.insert(result,w)
    end)
    return result
end

local function completion(str)
    local ch = str:sub(1,1)
    if ch == "r" then
        return {"reload"}
    elseif ch == "s" then
        return {"stop"}
    elseif ch == "m" then
        return {"mem","mem_dump"}
    elseif ch == "g" then
        return {"gc"}
    elseif ch == "y" then
        return {"yes"}
    elseif ch == "n" then
        return {"no"}
    elseif ch == "d" then
        return {"dump_model"}
    end
end

function _M.readline(prompt,func)
    return util_core.readline(prompt or ">>",func or completion)
end


return _M