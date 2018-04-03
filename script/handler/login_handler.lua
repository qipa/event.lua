local protocol = require "protocol"
local protocol_forward = import "server.protocol_forward"

function client_forward(args)
	local client_id = args.client_id
	local message_id = args.message_id
	local data = args.data
	local name = protocol_forward.forward[message_id]
	local message = protocol.decode[name](data)
	table.print(message,"client_forward")
end