#!/bin/bash

proc=("logger" "world" "login" "scene" "agent")

user=`whoami`
uid=`./lua ./script/common/env_reader.lua uid`


ps -C -elf -U${user} |grep @0*$uid|awk '{print $1,$4}'|while read line
do
	for name in ${proc[@]};do
		finded=`echo -e $line | grep -c $name`
		if [[ $finded > 0 ]];then
			echo "${line} is running"
			# pid=`echo $line|awk '{print $1}'`
			# kill -9 $pid
		fi
	done
done

result=`ps -C -elf -U${user} |grep @0*$uid|awk '{print $1,$4}'|grep login|wc -l`

if [ $result -ne "0" ];then
	echo "login server is running,try to notify stop"
	./event console@stop
	echo $?
	if [[ $? != 0 ]];then
		echo "server recive stop"
	else
		echo "server alreay recive stop before,please wait for second"
	fi
fi
