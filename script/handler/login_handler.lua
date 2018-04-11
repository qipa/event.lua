local protocol = require "protocol"
local cjson = require "cjson"
local util = require "util"
local protocol_forward = import "server.protocol_forward"

function client_forward(args)
	local client_id = args.client_id
	local message_id = args.message_id
	local data = args.data
	local name = protocol_forward.forward[message_id]
	local message = protocol.decode[name](data)
	table.print(message,"client_forward")
end

function auth(client_id,args)
	local account = args.account

end

function enter(client_id,args)
	local account = args.account
	local user_id = args.user_id
	local json = cjson.encode({account = account,user_id = user_id})
	local md5 = util.md5(json)
	local now = util.time()
	local token = util.rc4(json,now)

end