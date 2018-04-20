local event = require "event"
local util = require "util"
local model = require "model"
local protocol = require "protocol"
local mongo = require "mongo"
local route = require "route"
local channel = require "channel"
local cjson = require "cjson"
local http = require "http"
local startup = import "server.startup"
local server_manager = import "module.server_manager"


model.register_binder("scene_channel","id")
model.register_binder("agent_channel","id")


local common_channel = channel:inherit()
function common_channel:disconnect()
	if self.id ~= nil then
		if self.name == "agent" then
			server_manager:agent_server_down(self.id)
		elseif self.name == "scene" then
			server_manager:scene_server_down(self.id)
		end
	end
end

local function channel_accept()

end

event.fork(function ()
	startup.run()

	local ok,reason = event.listen(env.master,4,channel_accept,common_channel)
	if not ok then
		event.breakout(reason)
		return
	end
	local httpd,reason = http.listen(env.master_http,function (channel,method,url,header,body)
		local split = url:split("/")
		local func = server_manager[split[#split]]
		if not func then
			channel:reply(404,"not found")
			return
		end
		local result = func(channel,body)
		local content = cjson.encode(result)
        channel:reply(200,content)
    end)
    if not httpd then
        event.error(string.format("master server listen http:%s failed",env.master_http))
        os.exit(1)
    end
    import "handler.master_handler"
end)
