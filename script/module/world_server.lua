local model = require "model"
local database_common = import "module.database_common"


function start(self)
	local db_channel = model.get_db_channel()
	db_channel:db("common")
	local guild_info = db_channel:findAll("guild")
	for _,info in pairs(guild_info) do

	end
end

function stop(self)
	database_common:flush()
end

