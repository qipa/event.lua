#!/bin/bash

cd ..

type mongod >/dev/null 2>&1 || { echo "mongod require,but not install in system";exit 1; }

function read_env()
{
	result=`./lua ./script/common/env_reader.lua $1`
	echo $result
}

addr=$(read_env mongodb)
if [ $addr == "nil" ];then
	echo "mongo addr not found in .env"
	exit 1
fi

port=$(echo $addr|grep -P ':[0-9]+' -o|grep -P '[0-9]+' -o)

path=`pwd`
conf_path=${path}/mongo_data/mongod.conf

user=`whoami`

# result=`ps -U${user} -elf|grep mongod|grep -v grep|grep -c ${conf_path}`
result=$(ps -U${user} -elf|grep mongod|grep ${conf_path}|grep -v grep|wc -l)

if [ $result -eq 0 ];then
	if [ ! -d "./mongo_data" ];then
		mkdir ./mongo_data
	fi

	if [ ! -d "./mongo_data/data" ];then
		mkdir ./mongo_data/data
	fi

	mongod_conf="./mongo_data/mongod.conf"
	if [ ! -f $mongod_conf ];then
		touch $mongod_conf
		echo "dbpath=${path}/mongo_data/data" >> $mongod_conf
		echo "logpath=${path}/mongo_data/mongod.log" >> $mongod_conf
		echo "logappend=true" >> $mongod_conf
		echo "fork=true" >> $mongod_conf
		echo "port=${port}" >> $mongod_conf
		echo "pidfilepath=${path}/mongo_data/mongod.pid" >> $mongod_conf
	fi

	mongod -f ${path}/mongo_data/mongod.conf &
	exit 0
else
	pid=`ps -U${user} -elf|grep mongod|grep ${conf_path}|grep -v grep|awk '{print $4}'`
	echo "mongod:${pid} already running"
	exit 1
fi
