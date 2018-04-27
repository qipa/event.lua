


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

return _M