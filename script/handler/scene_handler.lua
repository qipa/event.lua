local scene_user = import "module.scene_user"


function __init__(self)

end


function enter_scene(_,args)
	local fighter = scene_user.cls_scene_user:unpack(args.fighter)

end

function leave_scene(_,args)

end