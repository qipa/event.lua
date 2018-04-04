local event = require "event"
local persistence = require "persistence"


local fs = persistence:open("test")

event.fork(function ()

	local count = 1
	while true do
		local data = {
			a = count,
			b = {
				age = 11,
				num = 1.123
			},
			c = "mrq"
		}
		fs:save("mrq",data)
		event.sleep(0.01)
		count = count + 1
	end

end)
