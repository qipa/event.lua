#!/bin/bash

cd ..

if [ ! -f "./event" ];then
	make
fi

function read_env()
{
	result=`./lua ./script/common/env_reader.lua $1`
	echo $result
}

proc=("logger" "world" "login" "scene" "agent")

user=`whoami`
uid=$(read_env "uid")

ps -C -elf -U${user} |grep @0*$uid|awk '{print $1,$4}'|while read line
do
	for name in ${proc[@]};do
		finded=`echo -e $line | grep -c $name`
		if [[ $finded > 0 ]];then
			echo "${line} is running"
		fi
	done
done

result=`ps -C -elf -U${user} |grep @0*$uid|awk '{print $1,$4}'|wc -l`

#仍然有此服务器id的集群进程在，停此启动
if [ $result -ne "0" ];then
	echo "server is running,please stop first"
	exit 1
fi

#data目录必须先建立，主要做本地数据保存，比如玩家唯一id
if [ ! -d "./data" ];then
	mkdir ./data
fi

#建立日志文件目录
log_path=$(read_env "log_path")
if [ $log_path != "nil" ];then
	if [ ! -d $log_path ];then
		mkdir $log_path
	fi
fi

#集群id记录表,每次重启都要册掉
if [ -f "./data/master/dist_id" ];then
	rm -rf ./data/master/dist_id
fi

#删除所有无用的ipc文件
files=$(ls *.ipc 2> /dev/null | wc -l)
if [ "$files" != "0" ]; then
	rm ./*.ipc
fi

#把所有依赖的so文件路径添加到进程环境变量里
libev_path=`cd ./3rd/libev/.libs && pwd`
export LD_LIBRARY_PATH=${libev_path}":"$LD_LIBRARY_PATH

config=$(read_env "config")
protocol=$(read_env "protocol")
log_path=$(read_env "log_path")
login_addr=$(read_env "login_addr")
mongodb_addr=$(read_env "mongodb")
agent_num=$(read_env "agent_num")
scene_num=$(read_env "scene_num")

echo "server uid:${uid}"
echo "server config path:${config}"
echo "server protocol path:${protocol}"
echo "server log path:${log_path}"
echo "server login addr:${login_addr}"
echo "server mongodb addr:${mongodb_addr}"

#服务器集群初始化临时进程
#主要建立表索引，做一些校验工作
./event console@startup

if [[ $? != 0 ]];then
	exit 0
fi

./event server/logger &
echo "logger server start"

./event server/login &
echo "login server start"

./event server/world &
echo "world server start"

i=0
while [ $i -lt $scene_num ]
do
	./event server/scene &
	echo "scene server start"
	let i++
done

i=0
while [ $i -lt $agent_num ]
do
	./event server/agent &
	echo "agent server start"
	let i++
done


# gdb --args ./event server/logger  server/world  server/login server/agent server/agent server/scene server/scene server/scene 
