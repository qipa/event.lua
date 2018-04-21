local event = require "event"



local function main()
	print("!run")
	event.sleep(1)
end


local mutex = event.mutex()
event.fork(function ()
	for i = 1,1024 do
		event.fork(function ()
			print("run 1",i)
			mutex(main)
			print("run 2",i)
		end)
		
	end
end)
