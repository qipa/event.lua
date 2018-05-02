local event = require "event"
local cjson = require "cjson"
local model = require "model"
local util = require "util"



function __init__(self)
	
end

function init(user)
	user.task_mgr = {}
	user:dirty_field("task_mgr")
end

function accept(user)


end

function submit(user)

end

