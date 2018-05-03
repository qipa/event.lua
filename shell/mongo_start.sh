#!/bin/bash

cd ..

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
		echo "port=10105" >> $mongod_conf
		echo "pidfilepath=${path}/mongo_data/mongod.pid" >> $mongod_conf
	fi

	mongod -f ${path}/mongo_data/mongod.conf &
else
	pid=`ps -U${user}|grep mongod|awk '{print $1}'`
	echo "mongod:${pid} already running"
fi
