#!/bin/bash

cd ..

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
	if [[ $? != 0 ]];then
		echo "server recive stop"
	else
		echo "server alreay recive stop before,please wait for second"
	fi
fi

count=0
while(( $count<=60 ))
do
world=`ps -C -elf -U${user} |grep @0*$uid|awk '{print $1,$4}'|grep -c world`
if [[ $world > 0 ]];then
	world=`ps -C -elf -U${user} |grep @0*$uid|awk '{print $1,$4}'|grep world`
	echo "${world} still running,wait for stop"
	let "count++"
else
	ps -C -elf -U${user} |grep @0*$uid|awk '{print $1,$4}'|while read line
	do
		for name in ${proc[@]};do
			finded=`echo -e $line | grep -c $name`
			if [[ $finded > 0 ]];then
				pid=`echo $line|awk '{print $1}'`
				kill -9 $pid
			fi
		done
	done
	break
fi
sleep 1
done