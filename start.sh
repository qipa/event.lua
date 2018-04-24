
if [ ! -d "./data" ];then
	mkdir ./data
fi

if [ ! -d "./log" ];then
	mkdir ./log
fi

if [ -f "./data/master/dist_id" ];then
	rm -rf ./data/master/dist_id
fi

rm ./*.ipc

gdb --args ./event server/logger  server/world  server/login server/scene server/scene server/agent server/agent server/scene server/scene


# ./event server/logger &
# ./event server/center &
# ./event server/master &
# ./event server/world &
# ./event server/login &
# ./event server/scene &
# ./event server/scene &
# ./event server/agent &
# ./event server/agent &
