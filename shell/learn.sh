#!/bin/bash


arr=(1 2 5)
echo ${arr[@]} 	#返回整个数组
echo $arr 		#返回第一个原素
echo ${#arr[@]} #返回数组长度

#返回原素
for var in ${arr[@]};
do
	echo $var
done

#返回索引
for index in ${!arr[@]};
do
	echo $index ${arr[$index]}  #不能用$arr[$index],shell会解析成$arr [字符 $index ]字符
done

i=0
while [ $i -lt ${#arr[@]} ]
do
	echo ${arr[$i]}
	let i++
done

#[]为最原始的test，大部分shell都支持
#[[]]为更高始的test
#字符串判断
#=,!=,-n(长度大于0为true),-z(长度为0为true),str(非空为真)
#
str1="mrq"
if [ $str1 = "mr1q" ];then
	echo "= true"
else
	echo "= false"
fi

if [ $str1 != "mrq" ];then
	echo "!= true"
else
	echo "!= false"
fi 

str2="1"
if [ -z "$str2" ];then #空串为真,
	echo "-z true"
else
	echo "-z false"
fi 

str3="s"
if [ -n "$str3" ];then #非空串为真，条件里写上双引号才生效
	echo "-n true"
else
	echo "-n false"
fi 

str4="123"
if [ $str4 ];then  #长度大于0为真（未定义或者长度为o为false）
	echo "true"
else
	echo "false"
fi

#数字判断eq(==),-ne(!=),-gt(>),-ge(>=)
#-lt(<),-le(<=)
num1=2
if [ $num1 -eq 2 ];then 
	echo "-eq true"
else
	echo "-eq false"
fi

#文件
#-r(可读),-w(可写),-x(可执行)
#-f(普通文件),-d(目录),-s(大小非o)

if [ -f "./learn.sh" -o -f "./lefarn.sh" ];then
	echo "-f true"
else
	echo "-f false"
fi

#!非,-a 与,-o 或,每个字符后要有空格

./child.sh

if [ $? = "0" ];then
	echo "return 0"
else
	echo "return 1"
fi

num=0
ps|while read line
do
	echo $line
	# let num=num+1
	let num++
	echo "while read "$num
done
echo "out while read ${num}" #为0

i=0
while [ $i -lt 5 ]
do
	echo "while "$i
	let i++
done
echo "out while "$i #能返回正常结果

result=`ps|wc -l`
echo $result

result=`ps|awk '{print $1,$4}'` #没有回车的
echo "awk "$result

#函数利用echo返回结果
cd ..
function read_env()
{
	echo $1 $2 "@"
	result=`./lua ./script/common/env_reader.lua $1`
	echo $result
	return -256
}

uid=$(read_env uid 1) #会把所有函数里的echo输出都捕捉了
echo $?  #接收函数的返回值，函数return必须是数字-256~256
echo "uid ? ${uid}"

read_env "config" #直接调用
read_env config   #直接调用,等效于上

echo a,"a"

int=2
str="1"
if [ $int = $str ];then
	echo "true!"
else
	echo "false!"
fi


#&&命令返回值为0时为true,才执行后面的命令
type mongod && echo "mongod exit"
#||命令返回值为false,才执行后面的命令