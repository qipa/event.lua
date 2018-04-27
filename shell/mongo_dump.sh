#!/bin/bash

cd ..

dbs=("login_user" "agent_user" "world_user" "common")

if [ ! -d "./mongo_bak" ];then
	mkdir mongo_bak
fi

now=`date +%Y_%m_%d_%H_%M`
for db in ${dbs[@]};
do
	mongodump -h 127.0.0.1:10105 -d $db -o ./mongo_bak/$db
	tar zcvf ${db}_${now}.tar.gz  ./mongo_bak/$db/$db/
	mv ${db}_${now}.tar.gz ./mongo_bak/$db/
	rm -rf ./mongo_bak/$db/$db
done


