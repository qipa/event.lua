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

make cleanall

git add .
git commit -m "update"
git push origin master