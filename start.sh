
if [ -f "./data/master/dist_id" ];then
	rm -rf ./data/master/dist_id
fi

if [ $1 -eq 1 ];then
	gdb --args ./event server/logger server/center server/master server/world server/login server/scene server/agent server/agent
else
	gdb --args ./event server/logger server/center server/master server/world server/login server/scene server/agent server/agent
	# ./event server/logger &
	# ./event server/center &
	# ./event server/master &
	# ./event server/world &
	# ./event server/login &
	# ./event server/scene &
	# ./event server/scene &
	# ./event server/agent &
	# ./event server/agent &
fi
