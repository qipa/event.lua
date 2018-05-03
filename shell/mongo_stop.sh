#!/bin/bash


cd ..

function read_env()
{
	result=`./lua ./script/common/env_reader.lua $1`
	echo $result
}

proc=("logger" "world" "login" "scene" "agent")
uid=$(read_env "uid")
user=`whoami`
path=`pwd`

tmp_file=`mktemp`

count=0
ps -C -elf -U${user} |grep @0*$uid|awk '{print $1,$4}' > $tmp_file
while read line
do
	for name in ${proc[@]};do
		finded=`echo -e $line | grep -c $name`
		if [[ $finded > 0 ]];then
			echo "${line} is running"
			let count++
		fi
	done
	
done < $tmp_file

rm $tmp_file

if [ $count -ne 0 ];then
	echo "server still running,please stop first"
	exit 0
fi

mongod --shutdown --dbpath=$path/mongo_data/data