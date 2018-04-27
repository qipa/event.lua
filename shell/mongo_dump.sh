#!/bin/bash

cd ..

dbs=("login_user" "agent_user" "world_user" "common")

if [ ! -d "./mongo_bak" ];then
	mkdir mongo_bak
fi

now=`date +%Y_%m_%d_%H_%M`
for db in ${dbs[@]};
do
	mongodump -h 127.0.0.1:10105 -d $db -o ./mongo_bak/$now
	tar zcvf ${db}.tar.gz  ./mongo_bak/$now/$db/
	mv ${db}.tar.gz ./mongo_bak/$now/
	rm -rf ./mongo_bak/$now/$db/
done


