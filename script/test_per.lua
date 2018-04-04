local persistence = require "persistence"


local fs = persistence:open("test")
-- local data = {
-- 	a = 1,
-- 	b = {
-- 		age = 11,
-- 		num = 1.123
-- 	},
-- 	c = "mrq"
-- }
-- fs:save("mrq",data)
table.print(fs:load("mrq"))