local fastaoi = require "fastaoi.core"
local util = require "util"
local event = require "event"

local function distance(pos0,pos1)
	return math.sqrt((pos0.x - pos1.x) * (pos0.x - pos1.x) + (pos0.z - pos1.z) * (pos0.z - pos1.z))
end


local function test_simple_aoi()
	local core = fastaoi.new(1000,1000,2,5,1000)

	local objs = {}
	for i = 2,1000 do
		local x,z = math.random(1,999),math.random(1,999)
		local id = core:enter(i,x,z,0)
		objs[id] = {x = x,z = z,uid = i}
	end
	
	local id,enter = core:enter(1,100,100,0)
	local list = {}
	for _,id in pairs(enter) do
		list[id] = true
		print("enter",id,distance({x = 100,z = 100},objs[id]))
	end

	
	local now = util.time()
	-- for i = 1,1024 do
		for i = 1,849 do
			local x ,z = 100+i,100+i
			local enter,leave = core:update(0,x,z)
			-- if #enter ~= 0 then
			-- 	-- print("<=====")
			-- 	for _,id in pairs(enter) do
			-- 		assert(list[id] == nil)
			-- 		list[id] = true
			-- 		-- print("enter",id,distance({x = x,z = z},objs[id]))
			-- 	end
			-- end

			-- if #leave ~= 0 then
			-- 	-- print("=====>")
			-- 	for _,id in pairs(leave) do
			-- 		assert(list[id] ~= nil)
			-- 		list[id] = nil
			-- 		-- print("leave",id,distance({x = x,z = z},objs[id]))
			-- 	end
			-- end

		end
	-- end

	for id in pairs(list) do
		print(id,distance({x = 949,z = 949},objs[id]))
	end

	print('simple aoi time', (util.time() - now) / 100)

end

event.fork(function ()
	test_simple_aoi()
	event.sleep(1)
	event.breakout()
end)
