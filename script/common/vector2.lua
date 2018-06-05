


local _M = {}


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

function _M.rotation(x,z,center_x,center_z,angle)
	local radian = math.rad(angle)
	local sin = math.sin(radian)
	local cos = math.cos(radian)
	local rx = (x - center_x) * cos - (z - center_z) * sin + center_x
	local rz = (x - center_x) * sin - (z - center_z) * cos + center_z
	return rx,rz
end

function _M.inside_circle(x,z,center_x,center_z,range)
	local dx = center_x - x
	local dz = center_z - z

	if math.abs(dx) > range or math.abs(dz) > range then
		return false
	end
	return dx * dx + dz * dz <= range * range
end

function _M.inside_sector(x,z,center_x,center_z,range,toward_angle,degree)
	if not _M.inside_circle(x,z,center_x,center_z,range)
		return false
	end

	local dx = x - center_x
	local dz = z - center_z

	local z_angle = math.deg(math.atan2(dx,dz)) - toward_angle + degree / 2

	while z_angle > 360 do
		z_angle = z_angle - 360
	end

	while z_angle < 0 do
		z_angle = z_angle + 360
	end

	return z_angle <= degree
end

function _M.inside_rectangle(x,z,center_x,center_z,toward_angle,length,width)
	local dx = x - center_x
	local dz = z - center_z

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

return _M