local protocol = require "protocol"


function __init__(self)
	print("self",self)
	protocol.handler["c2s_auth"] = req_auth
end

function __reload__(self)

end

function req_auth(user,args)


end