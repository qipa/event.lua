local event = require "event"
local cjson = require "cjson"
local model = require "model"
local util = require "util"



function __init__(self)
	
end

function init(self,user)
	if not user.task_mgr then
		user.task_mgr = {uid = user.uid,task_info = {}}
		user:dirty_field("task_mgr")
	end
	
	user:register_event(self,"ENTER_GAME","enter_game")
	user:register_event(self,"LEAVE_GAME","leave_game")
end

function enter_game(self,user)

end

function leave_game(self,user)
	user:deregister_event(self,"ENTER_GAME")
	user:deregister_event(self,"LEAVE_GAME")
end

function accept(self,user)


end

function submit(self,user)

end

