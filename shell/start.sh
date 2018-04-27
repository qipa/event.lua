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

if [ $result -ne "0" ];then
	echo "server is running,please stop first"
	exit 1
fi

if [ ! -d "./data" ];then
	mkdir ./data
fi

log_path=$(read_env "log_path")
if [ $log_path != "nil" ];then
	if [ ! -d $log_path ];then
		mkdir $log_path
	fi
fi

if [ -f "./data/master/dist_id" ];then
	rm -rf ./data/master/dist_id
fi

files=$(ls *.ipc 2> /dev/null | wc -l)
if [ "$files" != "0" ]; then
	rm ./*.ipc
fi

libev_path=`cd ./3rd/libev/.libs && pwd`
export LD_LIBRARY_PATH=${libev_path}":"$LD_LIBRARY_PATH

config=$(read_env "config")
protocol=$(read_env "protocol")
log_path=$(read_env "log_path")
login_addr=$(read_env "login_addr")
mongodb_addr=$(read_env "mongodb")

echo "server uid:${uid}"
echo "server config path:${config}"
echo "server protocol path:${protocol}"
echo "server log path:${log_path}"
echo "server login addr:${login_addr}"
echo "server mongodb addr:${mongodb_addr}"


./event console@index

if [[ $? != 0 ]];then
	echo "mongod server:${mongodb_addr} not start"
	exit 0
fi


./event server/logger &
./event server/login &
./event server/world &
./event server/scene &
./event server/scene &
./event server/scene &
./event server/agent &
./event server/agent &


# gdb --args ./event server/logger  server/world  server/login server/agent server/agent server/scene server/scene server/scene 
