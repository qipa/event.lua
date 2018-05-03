local event = require "event"
local cjson = require "cjson"
local model = require "model"
local util = require "util"



function __init__(self)
	
end

function init(user)
	if not user.task_mgr then
		user.task_mgr = {}
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

function accept(user)


end

function submit(user)

end

