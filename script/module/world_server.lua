local model = require "model"



function start(self)
	local db_channel = model.get_db_channel()
	db_channel:set_db("common")
	local guild_info = db_channel:findAll("guild")
	for _,info in pairs(guild_info) do

	end
end

function stop(self)
	database_common:flush()
end

