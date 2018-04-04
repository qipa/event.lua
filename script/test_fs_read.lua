local event = require "event"
local persistence = require "persistence"


local fs = persistence:open("test")

event.fork(function ()

	local count = 1
	while true do
		table.print(fs:load("mrq"))
		event.sleep(1)

	end

end)
