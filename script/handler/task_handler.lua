local util = require "util"
local protocol = require "protocol"
local event = require "event"
local model = require "model"

local task_manager = import "module.task_manager"

function __init__(self)
	protocol.handler["c2s_task_accept"] = req_accept
	protocol.handler["c2s_task_submit"] = req_submit
end

function req_accept(user,args)
	task_manager.accept(user)
end

function req_submit(user,args)
	task_manager.submit(user)
end