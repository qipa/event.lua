local event = require "event"
local cjson = require "cjson"
local model = require "model"
local util = require "util"
local protocol = require "protocol"

local fighter = import "module.fighter"
local scene_server = import "module.scene_server"
local server_manager = import "module.server_manager"

cls_scene_user = fighter.cls_fighter:inherit("monster")

function __init__(self)

end

function cls_scene_user:init(data)
	fighter.cls_fighter.init(self,data)
end

function cls_scene_user:destroy()
	
end

function cls_scene_user:do_enter(scene_uid)
	fighter.cls_fighter.do_enter(self,scene_uid)
end

function cls_scene_user:do_leave()
	fighter.cls_fighter.do_leave(self)
end

