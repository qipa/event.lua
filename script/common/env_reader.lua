


local function dump(data, prefix, depth, output, record)
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
        output(tab..tostring(data))
        return
    end

    assert(record[data] == nil)
    record[data] = true

    local count = 0
    for k,v in pairs(data) do
        if type(v) == "table" then
            output(tab..tostring(k).."->")
            _M.dump(v, prefix, depth + 1, output, record)
        else
            output(tab..tostring(k).."->"..tostring(v))
        end
        count = count + 1
    end

    if count == 0 then
        output(tab.." {}")
    end
end


local k = ...

local FILE = assert(io.open("./.env","r"))

local env = {}

assert(load(FILE:read("*a"),"env","text",env))()

if type(env[k]) == "table" then
	dump(env[k])
else
	print(env[k])
end
