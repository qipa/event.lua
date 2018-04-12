local model = require "model"



function register(channel,args)
	channel.name = args.name
	model.bind_channel_witch_name(channel.name,channel)
end




