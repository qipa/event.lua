#!/bin/bash

cd ..

# if [ -d "./data" ];then
# 	rm -rf ./data
# fi

# if [ -d "./log" ];then
# 	rm -r ./log
# fi

# if [ -f "./data/master/dist_id" ];then
# 	rm -rf ./data/master/dist_id
# fi

# files=$(ls *.ipc 2> /dev/null | wc -l)
# if [ "$files" != "0" ]; then
# 	rm ./*.ipc
# fi

# make cleanall

rm -rf ./luaclib/convert/*.o
rm -rf ./src/*.o

git add ./luaclib/
git add ./src
git add ./lualib/
git add ./script/
git add ./shell/
git commit -m "update"
git push origin master