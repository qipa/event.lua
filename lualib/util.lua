require("lfs")
local event = require "event"
local util_core = require "util.core"

local type = type
local assert = assert
local os_date = os.date
local os_time = os.time
local math_modf = math.modf

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

function _M.to_date(unix_time)
    return os.date("*t",unix_time)
end

function _M.to_unixtime(year,mon,day,hour,min,sec)
    local time = {
        year = year,
        month = mon,
        day = day,
        hour = hour,
        min = min,
        sec = sec,
    }
    return os.time(time)
end

function _M.format_to_unixtime(str)
    local year,mon,day,hour,min,sec = string.match(str,"(.*)-(.*)-(.*) (.*):(.*):(.*)")
    return _M.to_unixtime(year,mon,day,hour,min,sec)
end

function _M.format_to_daytime(unix_time,str)
    local hour,min = string.match(str,"(%d+):(%d+)")
    return _M.daytime(unix_time,tonumber(hour),tonumber(min))
end

function _M.daytime(unix_time,hour,min,sec)
    local unix_time = unix_time + 8 * 3600
    local tmp = math_modf(unix_time / 86400)
    local result = tmp * 86400 - 8 * 3600
    if hour then
        result = result + hour * 3600
    end
    if min then
        result = result + min * 60
    end
    if sec then
        result = result + sec
    end
    return result
end

function _M.next_time(unix_time,sec)
    return _M.daytime(unix_time) + sec
end

function _M.day_start(unix_time)
    return _M.daytime(unix_time)
end

function _M.day_over(unix_time)
    return _M.daytime(unix_time) + 24 * 3600
end

function _M.week_start(unix_time)
    local day_start = _M.day_start(unix_time)
    local result = os.date("*t",day_start)

    local wday = result.wday
    if wday == 2 then
        return day_start
    end

    if wday == 1 then
        wday = 8
    end
    return day_start - (wday-2) * 24 * 3600
end

function _M.week_over(unix_time)
    return _M.week_start(unix_time) + 7 * 24 * 3600
end

function _M.same_day(ti0,ti1,sep)
    assert(ti0 ~= nil and ti1 ~= nil)
    local sep = sep or 0
    return _M.daytime(ti0 - sep) == _M.daytime(ti1 - sep)
end

function _M.same_week(ti1,ti2,sep)
    local ti1 = ti1 - (sep or 0)
    local ti2 = ti2 - (sep or 0)

    local wstart
    if ti1 < ti2 then
        wstart = _M.week_start(ti2)
        if ti1 < wstart then
            return false
        end
    else
        wstart = _M.week_start(ti1)
        if ti2 < wstart then
            return false
        end
    end
    return true
end

function _M.time_diff(desc,func)
    local now = _M.time()
    func()
    print(string.format("%s:%f",desc,(_M.time() - now) * 10))
end

--十进制右边数起第b位
function _M.decimal_bit(value,b)
    local l,r = math.modf((value % (10^b)) / (10^(b-1)))
    return l
end

--十进制右边数起第from到to位的数字
function _M.decimal_sub(value,from,to)
    local result = 0
    for i=from,to do
        local var = _M.decimal_bit(value,i)
        result = result + var * 10^(i-from)
    end
    local l,r = math.modf(result)
    return l
end

--vector2
function _M.normalize(x,z)
    local dt = math.sqrt(x * x + z * z)
    return x / dt, z / dt
end

function _M.lerp(from_x,from_z,to_x,to_z,ratio)
    return from_x + (to_x - from_x) * ratio,from_z + (to_z - from_z) * ratio
end

function _M.distance(from_x,from_z,to_x,to_z)
    return math.sqrt((from_x - to_x)^2 + (from_z - to_z)^2)
end

function _M.move_forward(from_x,from_z,to_x,to_z,pass)
    local distance = _M.distance(from_x,from_z,to_x,to_z)
    local ratio = pass / distance
    if ratio > 1 then
        ratio = 1
    end
    return _M.lerp(from_x,from_z,to_x,to_z,ratio)
end

function _M.move_toward(from_x,from_z,dir,distance)
    local radian = math.atan2(dir.z / dir.x)
    local x = math.cos(radian) * distance + from_x
    local z = math.sin(radian) * distance + from_z
    return x,z
end

function _M.rotation(x,z,center_x,center_z,angle)
    local radian = math.rad(angle)
    local sin = math.sin(radian)
    local cos = math.cos(radian)
    local rx = (x - center_x) * cos - (z - center_z) * sin + center_x
    local rz = (x - center_x) * sin + (z - center_z) * cos + center_z
    return rx,rz
end

function _M.inside_circle(src_x,src_z,range,x,z)
    local dx = src_x - x
    local dz = src_z - z

    if math.abs(dx) > range or math.abs(dz) > range then
        return false
    end
    return dx * dx + dz * dz <= range * range
end

function _M.inside_sector(src_x,src_z,range,toward_angle,degree,x,z)
    if not _M.inside_circle(src_x,src_z,range,x,z) then
        return false
    end

    local dx = x - src_x
    local dz = z - src_z

    local z_angle = math.deg(math.atan2(dx,dz)) - toward_angle + degree / 2

    while z_angle > 360 do
        z_angle = z_angle - 360
    end

    while z_angle < 0 do
        z_angle = z_angle + 360
    end

    return z_angle <= degree
end

function _M.inside_rectangle(src_x,src_z,toward_angle,length,width,x,z)
    local dx = x - src_x
    local dz = z - src_z

    local z_angle = math.deg(math.atan2(dx,dz)) - toward_angle
    if z_angle >= 270 then
        z_angle = z_angle - 360
    elseif z_angle <= -270 then
        z_angle = z_angle + 360
    end

    if z_angle < -90 or z_angle > 90 then
        return false
    end

    local z_radian = math.rad(math.abs(z_angle))
    local dt = math.sqrt(dx * dx + dz * dz)
    local x_change = dt * math.cos(z_radian)
    local z_change = dt * math.sin(z_radian)

    if (x_change < 0 or x_change > length) or (z_change < 0 or z_change > width) then
        return false
    end
    return true
end

--vector2
local vector2 = {}
vector2.__index = vector2

function vector2:new(x,z)
    local vt = setmetatable({},self)
    vt[1] = x
    vt[2] = z
    return vt
end

function vector2:new(vt)
    return setmetatable(vt,self)
end

function vector2:__add(vt)
    local x = self[1] + vt[1]
    local z = self[2] + vt[2]
    return vector2:new(x,z)
end

function vector2:__sub(vt)
    local x = self[1] - vt[1]
    local z = self[2] - vt[2]
    return vector2:new(x,z)
end

function vector2:__mul(vt)
    local x = self[1] * vt[1]
    local z = self[2] * vt[2]
    return vector2:new(x,z)
end

function vector2:__div(vt)
    local x = self[1] / vt[1]
    local z = self[2] / vt[2]
    return vector2:new(x,z)
end

function vector2:__eq(vt)
    return self[1] == vt[1] and self[2] == vt[2]
end

function vector2:abs()
    local x = math.abs(self[1])
    local z = math.abs(self[2])
    return vector2:new(x,z)
end

function vector2:angle(vt)
    local dot = self:dot(vt)
    local cos = dot / (self:magnitude() * vt:magnitude())
    return math.deg(math.acos(cos))
end

function vector2:magnitude()
    return math.sqrt(self[1] * self[1] + self[2] * self[2])
end

function vector2:sqrmagnitude()
    return self[1] * self[1] + self[2] * self[2]
end

function vector2:normalize()
    local dt = math.sqrt(self[1] * self[1] + self[2] * self[2])
    return vector2:new(self[1] / dt,self[2] / dt)
end

function vector2:distance(to)
    return math.sqrt((self[1] - to[1])^2 + (self[2] - to[2])^2)
end

function vector2:lerp(to,t)
    local x = self[1] + (to[1] - self[1]) * t
    local z = self[2] + (to[1] - self[2]) * t
    return vector2:new(x,z)
end

function vector2:move_forward(vt,pass)
    local dt = self:distance(vt)
    local t = pass / dt
    if t > 1 then
        t = 1
    end
    return self:lerp(vt,t)
end

function vector2:move_toward(dir,dt)
    local radian = math.atan2(dir[2] / dir[1])
    local x = math.cos(radian) * dt + self[1]
    local z = math.sin(radian) * dt + self[2]
    return vector2:new(x,z)
end

function vector2:rotation(center,angle)
    local radian = math.rad(angle)
    local sin = math.sin(radian)
    local cos = math.cos(radian)
    local rx = (self[1] - center[1]) * cos - (self[2] - center[2]) * sin + center[1]
    local rz = (self[1] - center[1]) * sin + (self[2] - center[2]) * cos + center[2]
    return vector2:new(rx,rz)
end

function vector2:dot(vt)
    return self[1] * vt[1] + self[2] * vt[2]
end

function vector2:cross(vt)
    return self[1] * vt[2] - self[2] * vt[1]
end

_M.vector2 = vector2

function _M.dot2segment(src_x,src_z,u_x,u_z,x,z)
    local pt_over = vector2:new(x,z)
    local pt_start = vector2:new(src_x,src_z)
    local vt_u = vector2:new(u_x,u_z)
    
    local vt_src = pt_over - pt_start

    local t = vt_src:dot(vt_u) / vt_u:sqrmagnitude()

    if t < 0 then
        t = 0
    elseif t > 1 then
        t = 1
    end

    vt_u[1] = vt_u[1] * t
    vt_u[2] = vt_u[2] * t

    local result = (pt_over - (pt_start + vt_u)):sqrmagnitude()
    return result
end

return _M