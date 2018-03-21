local event = require "event"
local mongo = require "mongo"
local helper = require "helper"

event.fork(function ()
	local channel = event.connect("tcp://127.0.0.1:10105",4,mongo)
	channel:init("u3d")

	-- for i = 1,1024*1024 do
	-- 	channel:insert("test",{id = i,name = "mrq"..i})
	-- end


	for i = 1,10 do
		print("!!!!")
		-- channel:findOne("test",{query = {id = i}},function (data)
		-- 	print("data",data.id)
		-- end)

		local data =  channel:findOne("test",{query = {id = i}})
		print("data",data.id)
	end
	

end)
