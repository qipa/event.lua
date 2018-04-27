


local _M = {}


function _M.lerp(from_x,from_z,to_x,to_z,ratio)
	return from_x + (to_x - from_x) * ratio,from_z + (to_z - from_z) * ratio
end

function _M.distance(from_x,from_z,to_x,to_z)
	return math.sqrt((from_x - to_x)^2 + (from_z - to_z)^2)
end