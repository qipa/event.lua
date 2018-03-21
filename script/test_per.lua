local persistence = require "persistence"


local file_system = persistence:open("test")
local data = {
	a = 1,
	b = {
		age = 11,
		num = 1.123
	},
	c = "mrq"
}
file_system:save("mrq",data)
-- table.print(file_system:load("mrq"))