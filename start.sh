
if [ ! -d "./data" ];then
	mkdir ./data
fi

if [ ! -d "./log" ];then
	mkdir ./log
fi

if [ -f "./data/master/dist_id" ];then
	rm -rf ./data/master/dist_id
fi

gdb --args ./event server/logger server/center server/master server/world server/login server/scene server/agent server/agent
