#!/bin/bash

if [ ! -d "./data" ];then
	mkdir ./data
fi

if [ ! -d "./log" ];then
	mkdir ./log
fi

if [ -f "./data/master/dist_id" ];then
	rm -rf ./data/master/dist_id
fi

files=$(ls *.ipc 2> /dev/null | wc -l)
if [ "$files" != "0" ]; then
	rm ./*.ipc
fi

proc=("logger" "world" "login" "scene" "agent")

user=`whoami`
uid=`./lua ./script/common/env_reader.lua uid`


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


# gdb --args ./event server/logger  server/world  server/login server/agent server/agent server/scene server/scene server/scene 


./event server/logger &
./event server/world &
./event server/login &
./event server/scene &
./event server/scene &
./event server/scene &
./event server/agent &
./event server/agent &
