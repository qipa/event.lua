local util = require "util"
local event = require "event"
local mongo = require "mongo"
local model = require "model"

model.register_value("db_channel")
local db_channel,reason = event.connect(env.mongodb,4,true,mongo)
if not db_channel then
	print(string.format("%s connect db:%s faield:%s",env.name,env.mongodb,reason))
	os.exit()
end

model.set_db_channel(db_channel)

event.fork(function ()
	local buidler = import "module.id_builder"
buidler:init(1)

util.time_diff("build id",function ()
	for i = 1,1024*1024*10 do
		buidler.alloc_user_uid()
	end
end)

end)
