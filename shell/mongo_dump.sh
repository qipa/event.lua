#!/bin/bash

cd ..

dbs=("login_user" "agent_user" "world_user" "common")

if [ ! -d "./mongo_bak" ];then
	mkdir mongo_bak
fi

cd mongo_bak
now=`date +%Y_%m_%d_%H_%M`
for db in ${dbs[@]};
do
	mongodump -h 127.0.0.1:10105 -d $db -o ./$now
	cd ./$now
	tar zcvf ${db}.tar.gz  ./$db
	rm -rf ./$db/
	cd ..
done


