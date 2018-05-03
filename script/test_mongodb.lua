local event = require "event"
local mongo = require "mongo"

local db_channel,reason = event.connect("tcp://127.0.0.1:10105",4,true,mongo)
if not db_channel then
	os.exit()
end
db_channel:init("sunset")

-- for i = 1,10 do
-- 	db_channel:insert("role",{name = "mrq",id = i})
-- end

local updater = {}
updater["$set"] = {["list.a"] = 3}
db_channel:update("role",{account = "m111q"},updater,1)